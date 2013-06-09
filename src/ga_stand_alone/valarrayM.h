#ifndef VALARRAYM_
#define VALARRAYM_

#include <iostream>
#include <valarray>


/******************************************************************************/
/*                              VALARRAY WRAPPER                              */
/*     overrides copy constructor and assignment operator with resizing       */
/******************************************************************************/

template <class T>
class valarrayM: public valarray<T>
  {
    public:
      valarrayM() : valarray<T>() {}
      valarrayM( size_t n ) : valarray<T>( n ) { }
      valarrayM& operator=( const valarrayM& v )  // called N*(maxGenerations+1) times
        {
          if ( &v != this )
            {
              if ( this->size() != v.size() )
                this->resize( v.size() );
              for ( int i=0; i<this->size(); i++ ) this->operator[]( i ) = v[i];
              // ( reinterpret_cast< valarray<T>* >( this ))->operator=( *( reinterpret_cast< valarray<T>* >( &v ) ) );
            }
        }
      valarrayM( const valarrayM& v ) // called N times
        {
          if ( this->size() != v.size() )
            this->resize( v.size() );
          for ( int i=0; i<this->size(); i++ )
            this->operator[]( i ) = v[i];
        }
      
  };

  
typedef valarrayM<float> FloatVector;
typedef valarrayM<double> DoubleVector;


ostream& operator<<( ostream& os, const FloatVector& r )
  {
    std::cout << "(";
    for ( int d=0 ;d<r.size()-1; d++ )
      std::cout << r[d] << ", ";
    std::cout << r[r.size()-1] << ")";
  }

ostream& operator<<( ostream& os, const DoubleVector& r )
  {
    std::cout << "(";
    for ( int d=0 ;d<r.size()-1; d++ )
      std::cout << r[d] << ", ";
    std::cout << r[r.size()-1] << ")";
  }
////////////////////////////////////////////////////////////////////////////////

#endif
