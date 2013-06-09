#include <iostream>
using namespace std;

#include <fstream>
#include <string>

int main()
  {
    ifstream fin( "dataset2new.txt" );
    ofstream fout1( "uno.txt" );
    ofstream fout2( "due.txt" );
    ofstream fout3( "tre.txt" );
    ofstream fout4( "quattro.txt" );
    ofstream fout5( "cinque.txt" );
    while ( !fin.eof() )
      {
        string s;
        getline( fin, s );
        fout1 << s << "\n";
        getline( fin, s );
        fout2 << s << "\n";
        getline( fin, s );
        fout3 << s << "\n";
        getline( fin, s );
        fout4 << s << "\n";
        getline( fin, s );
        fout5 << s << "\n";
      }
  }
