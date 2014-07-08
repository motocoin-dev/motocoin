#ifndef DEBUG_H
#define DEBUG_H


#ifdef QT_DEBUG
#define DEBUG_MSG(str) do { /*cout<< str << std::endl;*/ } while( false )
#else
#include <iostream>
#define DEBUG_MSG(str) do { std::cout<<str << std::endl; } while( false )
#endif

#endif // DEBUG_H
