/*
 * (C) Copyright 1996-2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/eckit.h"

#include <limits.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <dirent.h>
#include <sys/statvfs.h>

#include "eckit/compat/StrStream.h"
#include "eckit/config/Resource.h"
#include "eckit/filesystem/BasePathNameT.h"
#include "eckit/io/FileHandle.h"
#include "eckit/filesystem/LocalPathName.h"
#include "eckit/io/PartFileHandle.h"
#include "eckit/filesystem/marsfs/MarsFSPath.h"
#include "eckit/io/Length.h"
#include "eckit/io/StdFile.h"
#include "eckit/io/cluster/ClusterDisks.h"
#include "eckit/io/cluster/NodeInfo.h"
#include "eckit/log/Bytes.h"
#include "eckit/log/TimeStamp.h"
#include "eckit/os/Stat.h"
#include "eckit/runtime/Context.h"
#include "eckit/thread/AutoLock.h"
#include "eckit/thread/Mutex.h"
#include "eckit/utils/Regex.h"

//-----------------------------------------------------------------------------

namespace eckit {

//-----------------------------------------------------------------------------

static Mutex local_mutex;

// I need to come back here when we have a proper std::string class

LocalPathName LocalPathName::baseName(bool ext) const
{
    const char *q = path_.c_str();
    int n = -1;
    int i = 0;

    while(*q) {
        if(*q == '/') n = i;
        q++;
        i++;
    }

    std::string s(path_.c_str() + n + 1);
    if(!ext)
    {
         n = -1;
         i = 0;
         q = s.c_str();
         while(*q) {
             if(*q == '.') { n = i; break; }
             q++;
             i++;
         }
         if(n>= 0) s.resize(n);
    }
    return s;

}

LocalPathName LocalPathName::dirName() const
{

    int n = -1;
    int i = 0;
    const char *q = path_.c_str();
    while(*q) {
        if(*q == '/') n = i;
        q++;
        i++;
    }

    switch(n)
    {
        case -1:
            return std::string(".");
            break;

        case 0:
            return std::string("/");
            break;

    }

    std::string s(path_);
    s.resize(n);
    return s;

}

BasePathName* LocalPathName::checkClusterNode() const
{
    std::string n = ClusterDisks::node(path_);
    if(n != "local") {
//        Log::warning() << *this << " is now on node [" << n << "]" << std::endl;
        return new BasePathNameT<MarsFSPath>(MarsFSPath(n,path_));
    }
    return new BasePathNameT<LocalPathName>(LocalPathName(path_));
}

LocalPathName LocalPathName::orphanName() const
{

	StrStream os;
	os << mountPoint()  << "/orphans/";

    const char *q = path_.c_str();
    while(*q) {
        if(*q == '/')  os << '_';
		else os << *q;
		q++;
    }

	os << StrStream::ends;

    std::string s(os);
    return s;

}

bool LocalPathName::exists() const
{
	return ::access(path_.c_str(),F_OK) == 0;
}

bool LocalPathName::available() const
{
	return true;
}

LocalPathName LocalPathName::cwd()
{
    AutoLock<Mutex> lock(local_mutex);
    char buf [PATH_MAX+1];
	if(!getcwd(buf, sizeof(buf)))
		throw FailedSystemCall("getcwd");
    return LocalPathName( buf );
}

LocalPathName LocalPathName::unique(const LocalPathName& path)
{

    AutoLock<Mutex> lock(local_mutex);

	static std::string format = "%Y%m%d.%H%M%S";

	static unsigned long long n = (((unsigned long long)::getpid()) << 32);

	std::string s;
	StrStream os;
	os << path << '.' << TimeStamp(format) << '.' << n++ << StrStream::ends;
	s = std::string(os);

	while(::access(s.c_str(),F_OK) == 0)
	{
		StrStream os;
		os << path << '.' << TimeStamp(format) << '.' << n++ << StrStream::ends;
		s = std::string(os);
	}

	LocalPathName result(s);
	result.dirName().mkdir();
	return result;
}

static void mkdir_if_not_exists( const char* path )
{
	Stat::Struct info;

	if( Stat::stat( path, &info) < 0 )
	{
		if( errno == ENOENT ) // no such file or dir
		{
			if(::mkdir(path,mode) < 0)
			{
				throw FailedSystemCall(std::string("mkdir ") + path);
			}
		}
		else // stat fails for unknown reason
		{
			throw FailedSystemCall( std::string("stat ") + path);
		}
	}

}

void LocalPathName::mkdir(short mode) const
{
	try
	{
		char path[MAXNAMLEN+1];
		zero(path);

		::strcpy( path,path_.c_str(),path_.length() );

		long l = strlen(path);

		for(long i=1; i < l; i++)
		{
			if(path[i] == '/')
			{
				path[i] = 0;

				mkdir_if_not_exists(path);

				path[i] = '/'; // put slash back
			}
		}

		mkdir_if_not_exists(path);
	}
	catch( FailedSystemCall& e )
	{
		Log::error() << "Failed to mkdir " << path_ << std::endl;
		throw;
	}
}

void LocalPathName::link(const LocalPathName& from,const LocalPathName& to)
{
	if(::link(from.c_str(),to.c_str()) != 0)
		throw FailedSystemCall(std::string("::link(") + from.path_ + ',' + to.path_ + ')');
}

void LocalPathName::rename(const LocalPathName& from,const LocalPathName& to)
{
	if(::rename(from.c_str(),to.c_str()) != 0)
		throw FailedSystemCall(std::string("::rename(") + from.path_ + ',' + to.path_ + ')');
}

void LocalPathName::unlink() const
{
	Log::info() << "Unlink " << path_ << std::endl;
	if(::unlink(path_.c_str()) != 0)
    {
        if(errno != ENOENT)
            throw FailedSystemCall(std::string("unlink ") + path_);
        else
            Log::info() << "Unlink failed " << path_ << Log::syserr << std::endl;
    }
}

void LocalPathName::rmdir() const
{
	Log::info() << "Rmdir " << path_ << std::endl;
	if(::rmdir(path_.c_str()) != 0)
    {
        if(errno != ENOENT)
            throw FailedSystemCall(std::string("rmdir ") + path_);
        else
            Log::info() << "Rmdir failed " << path_ << Log::syserr << std::endl;
    }
}


void operator<<(Stream& s,const LocalPathName& path)
{
	s << path.path_;
}

void operator>>(Stream& s,LocalPathName& path)
{
	s >> path.path_;
}

LocalPathName LocalPathName::fullName() const
{
	if(path_.length()  > 0 && path_[0] != '/')
    {
        char buf[PATH_MAX];
		return LocalPathName(std::string(getcwd(buf,PATH_MAX)) + "/" + path_);
    }

    return *this;
}


LocalPathName& LocalPathName::tidy()
{
	if(path_.length() == 0) return *this;

	if(path_[0] == '~')
	{
        path_ =  Context::instance().home() + "/" + path_.substr(1);
	}


	bool more = true;
	bool last = (path_[path_.length()-1] == '/');


	size_t p = 0;
	size_t q = 0;

	std::vector<std::string> v;

	while( (p = path_.find('/',q)) != std::string::npos )
	{
		v.push_back(std::string(path_,q,p-q));
		q = p+1;
	}

	v.push_back(std::string(path_,q));


	while(more)
	{
		more   = false;

		std::vector<std::string>::iterator i = v.begin();
		std::vector<std::string>::iterator j = i+1;


		while( j != v.end() )
		{
			if( (*j) == "" || (*j) == ".")
			{
				v.erase(j);
				more = true;
				break;
			}

			if( (*j) == ".." && (*i) != "..")
			{
				if ((*i) != "")
					*i = ".";
				v.erase(j);
				more = true;
				break;
			}

			++i;
			++j;

		}

	}

	path_ = "";

    for(size_t i = 0; i < v.size() ; i++)
	{
		path_ += v[i];
		if(i != v.size() - 1)
			path_ += '/';
	}

	if(last) path_ += '/';

	return *this;
}

class StdDir {
	DIR *d_;
public:
	StdDir(const LocalPathName& p) { d_ = opendir(p.localPath());}
	~StdDir()                 { if(d_) closedir(d_);    }
	operator DIR*()           { return d_;              }
};

void LocalPathName::match(const LocalPathName& root,std::vector<LocalPathName>& result,bool rec)
{
	// Note that pattern matching will only be done
	// on the base name.

	LocalPathName dir   = root.dirName();
	std::string   base  = root.baseName();


	// all those call should be members of LocalPathName

    //long len = base.length() * 2 + 2;

	Regex re(base,true);

	StdDir d(dir);

	if(d == 0)
	{
		Log::error() << "opendir(" << dir << ")" << Log::syserr << std::endl;
		throw FailedSystemCall(std::string("opendir(") + std::string(dir) + ")");
	}

	struct dirent buf;


	for(;;)
	{
		struct dirent *e;
#ifdef EC_HAVE_READDIR_R
		errno = 0;
		if(readdir_r(d,&buf,&e) != 0)
        {
			if(errno)
				throw FailedSystemCall("readdir_r");
			else
				e = 0;
        }
#else
		e = readdir(d);
#endif

		if(e == 0)
			break;

		if(re.match(e->d_name))
		{
			LocalPathName path = std::string(dir) + std::string("/") + std::string(e->d_name);
			result.push_back(path);
		}

		if(rec && e->d_name[0] != '.')
		{
			LocalPathName full = dir + "/" + e->d_name;
            Stat::Struct info;
            SYSCALL(Stat::stat(full.c_str(),&info));
			if(S_ISDIR(info.st_mode))
				match(full+"/"+base,result,true);
		}
	}

}

void LocalPathName::children(std::vector<LocalPathName>& files,std::vector<LocalPathName>& directories)
	const
{
	StdDir d(*this);

	if(d == 0)
	{
		Log::error() << "opendir(" << *this << ")" << Log::syserr << std::endl;
		throw FailedSystemCall("opendir");
	}

	struct dirent buf;


	for(;;)
	{
		struct dirent *e;
#ifdef EC_HAVE_READDIR_R
		errno = 0;
		if(readdir_r(d,&buf,&e) != 0)
        {
			if(errno)
				throw FailedSystemCall("readdir_r");
			else
				e = 0;
        }
#else
		e = readdir(d);
#endif

		if(e == 0)
			break;

		if(e->d_name[0] == '.')
			if(e->d_name[1] == 0 || (e->d_name[1] =='.' && e->d_name[2] == 0))
				continue;

		LocalPathName full = *this + "/" + e->d_name;
        Stat::Struct info;
        if(Stat::stat(full.c_str(),&info) == 0)
		{
			if(S_ISDIR(info.st_mode))
				directories.push_back(full);
			else
				files.push_back(full);
		}
		else Log::error() << "Cannot stat " << full << Log::syserr << std::endl;

	}
}

void LocalPathName::touch() const
{
	dirName().mkdir();
	StdFile f(*this,"a"); // This should touch the file
}

// This routine is used by TxnLog. It is important that
// the inode is preserved, otherwise ftok will give different
// result

void LocalPathName::empty() const
{
	StdFile f(*this,"w"); // This should clear the file
}

void LocalPathName::copy(const LocalPathName& other) const
{
	FileHandle in(*this);
	FileHandle out(other);
	in.saveInto(out);
}

void LocalPathName::backup() const
{
	copy(unique(*this));
}

Length LocalPathName::size() const
{
    Stat::Struct info;

    if(Stat::stat(path_.c_str(),&info) < 0)
		throw FailedSystemCall(path_);

	// Should ASSERT(is file)

	return info.st_size;
}

time_t LocalPathName::created() const
{
    Stat::Struct info;
    if(Stat::stat(path_.c_str(),&info) < 0)
		throw FailedSystemCall(path_);
	return info.st_ctime;
}

time_t LocalPathName::lastModified() const
{
    Stat::Struct info;
    if(Stat::stat(path_.c_str(),&info) < 0)
		throw FailedSystemCall(path_);
	return info.st_mtime;
}

time_t LocalPathName::lastAccess() const
{
    Stat::Struct info;
    SYSCALL(Stat::stat(path_.c_str(),&info));
	return info.st_atime;
}

bool LocalPathName::isDir() const
{
    Stat::Struct info;
    SYSCALL(Stat::stat(path_.c_str(),&info));
	return S_ISDIR(info.st_mode);
}

bool LocalPathName::sameAs(const LocalPathName& other) const
{
	if(!exists() || !other.exists())
		return false;

    Stat::Struct info1; SYSCALL(Stat::stat(path_.c_str(),&info1));
    Stat::Struct info2; SYSCALL(Stat::stat(other.path_.c_str(),&info2));
	return (info1.st_dev == info2.st_dev) && (info1.st_ino == info2.st_ino);
}

LocalPathName LocalPathName::realName() const
{
	char result[PATH_MAX+1];
	// This code is certainly machine dependant
	if(!::realpath(c_str(), result))
		throw FailedSystemCall(std::string("realpath ") + path_);
	return result;
#if 0
	char save[PATH_MAX+1];
	if(!getcwd(save,sizeof(save)))
		throw FailedSystemCall("getcwd");

	bool direct = isDir();

	if(direct)
	 	SYSCALL(::chdir(path_.c_str()));
	else
		SYSCALL(::chdir(dirName().path_.c_str()));

	char dir[PATH_MAX+1];
	if(!getcwd(dir,sizeof(dir)))
	{
		PANIC(::chdir(save) != 0); // We must go back
		throw FailedSystemCall("getcwd");
	}

	if(direct)
		return LocalPathName(dir);
	else
		return LocalPathName(std::string(dir) + "/" + baseName());
#endif

}

void LocalPathName::truncate(Length len) const
{
	SYSCALL(::truncate(path_.c_str(),len));
}

void LocalPathName::reserve(const Length& len) const
{
	ASSERT(!exists() || size() == Length(0));

	PartFileHandle part("/dev/zero",0,len);
	FileHandle     file(*this);

	Log::status() << "Reserving " << Bytes(len) << std::endl;
	part.saveInto(file);
}

void LocalPathName::fileSystemSize(FileSystemSize& fs) const
{
    struct statvfs d;
    SYSCALL(::statvfs(path_.c_str(),&d));
	long unavail = (d.f_bfree - d.f_bavail);
    fs.available = (unsigned long long)d.f_bavail*(unsigned long long)d.f_bsize;
    fs.total     = (unsigned long long)(d.f_blocks-unavail)*(unsigned long long)d.f_bsize;
}


DataHandle* LocalPathName::fileHandle(bool overwrite) const
{
    return new FileHandle(path_, overwrite);
}

DataHandle* LocalPathName::partHandle(const OffsetList& o, const LengthList& l) const
{
    return new PartFileHandle(path_, o, l);
}

DataHandle* LocalPathName::partHandle(const Offset& o, const Length& l) const
{
    return new PartFileHandle(path_, o, l);
}

LocalPathName LocalPathName::mountPoint() const
{
//	dev_t last;
    Stat::Struct s;
	LocalPathName p(*this);

	ASSERT(p.path_.length() > 0 && p.path_[0] == '/');

    SYSCALL2(Stat::stat(p.c_str(),&s),p);
	dev_t dev = s.st_dev;

	while(p != "/") {
		LocalPathName q(p.dirName());
        SYSCALL(Stat::stat(q.c_str(),&s));
		if(s.st_dev != dev)
			return p;
		p = q;
	}

	return p;
}

void LocalPathName::syncParentDirectory() const
{
    PathName directory = dirName();
#ifdef EC_HAVE_DIRFD
    Log::info() << "Syncing directory " << directory << std::endl;
    DIR *d = opendir(directory.localPath());
    if (!d) SYSCALL(-1);

    int dir;
    SYSCALL(dir = dirfd(d));
    int ret = fsync(dir);

    while (ret < 0 && errno == EINTR)
        ret = fsync(dir);

    if (ret < 0) {
        Log::error() << "Cannot fsync directory [" << directory << "]" << Log::syserr << std::endl;
    }

    ::closedir(d);
#else
    Log::info() << "Syncing directory " << directory << " (not supported)" << endl;
#endif
}

const std::string& LocalPathName::node() const
{
    return NodeInfo::thisNode().node();
}

const std::string& LocalPathName::path() const
{
    return path_;
}

std::string LocalPathName::clusterName() const
{
    StrStream os;
    os << "marsfs://" << node() << fullName() << StrStream::ends;
    return std::string(os);
}

//-----------------------------------------------------------------------------

} // namespace eckit

