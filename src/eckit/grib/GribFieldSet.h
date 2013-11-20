// File GribFieldSet.h
// Baudouin Raoult - (c) ECMWF Nov 13

#ifndef GribFieldSet_H
#define GribFieldSet_H

// namespace mpi;

// Headers
// #ifndef   machine_H
// #include "machine.h"
// #endif

// Forward declarations

// class ostream;

// 
#include "eckit/memory/Counted.h"
#include "eckit/filesystem/PathName.h"

namespace eckit {

class GribField;
class DataHandle;

class GribFieldMemoryStrategy;



class GribFieldSet {
public:



// -- Exceptions
	// None

// -- Contructors

    GribFieldSet();
    explicit GribFieldSet(GribField*);
    explicit GribFieldSet(size_t size);
    explicit GribFieldSet(const PathName& path);

    GribFieldSet(const GribFieldSet&);
    GribFieldSet(const vector<GribFieldSet>&);



// -- Destructor

	~GribFieldSet(); // Change to virtual if base class

// -- Convertors
	// None

// -- Operators

    GribFieldSet& operator=(const GribFieldSet&);

    GribFieldSet operator[](int i)  const;
    GribFieldSet slice(size_t from, size_t to, size_t step = 1) const;
    GribFieldSet slice(const vector<size_t>&) const;

    //====================================================================================


    //====================================================================================




// -- Methods

    void write(const PathName& path) const;
    void write(DataHandle& handle) const;

    void add(GribField*);


    size_t size() const { return fields_.size(); }
    const GribField* get(size_t i) const { return fields_[i]; }


    // Must be adopted 
    GribField* willAdopt(size_t i) const { return fields_[i]; }



    class iterator: public std::iterator<std::forward_iterator_tag, GribFieldSet> {

        std::vector<GribField*>::iterator j_;

        iterator& operator++(int);

    public:

        iterator(const std::vector<GribField*>::iterator& j): j_(j) {}

        bool operator!=(const iterator& other) const
            { return j_ != other.j_; }

        iterator& operator++() {
            ++j_;
            return *this;
        }

        GribFieldSet operator*() {
            return GribFieldSet(*j_);
        }

    };

    class const_iterator: public std::iterator<std::forward_iterator_tag, GribFieldSet> {

        std::vector<GribField*>::const_iterator j_;

        const_iterator& operator++(int);

    public:

        const_iterator(const std::vector<GribField*>::const_iterator& j): j_(j) {}

        bool operator!=(const const_iterator& other) const
            { return j_ != other.j_; }

        const_iterator& operator++() {
            ++j_;
            return *this;
        }

        GribFieldSet operator*() {
            return GribFieldSet(*j_);
        }

    };



    iterator begin() { return iterator(fields_.begin()); }
    iterator end()   { return iterator(fields_.end());   }

    const_iterator begin() const { return const_iterator(fields_.begin()); }
    const_iterator end()   const { return const_iterator(fields_.end());   }


// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods


    static void setStrategy(GribFieldMemoryStrategy&);

protected:

// -- Members
	// None

// -- Methods
	
    void print(ostream&) const; // Change to virtual if base class

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

private:

// No copy allowed

// -- Members
//
    std::vector<GribField*> fields_;

// -- Methods
	// None

// -- Overridden methods
	// None

// -- Class members
	// None

// -- Class methods
	// None

// -- Friends

    friend ostream& operator<<(ostream& s,const GribFieldSet& p)
        { p.print(s); return s; }

};

} // Namespace



#endif