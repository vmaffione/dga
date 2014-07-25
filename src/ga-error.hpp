#ifndef GA_ERROR_
#define GA_ERROR_

#include <iostream>
#include <string>

class GAError
{
    public:
        GAError(std::string s)
        {
            std::cout << "Error: " << s << "\n";
        }
};

#endif
