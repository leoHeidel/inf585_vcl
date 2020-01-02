#pragma once

#include <string>


namespace vcl
{
std::string backtrace(int const skip=1);
[[noreturn]] void call_error(std::string error, std::string message, std::string filename, std::string function_name, int line, std::string const& trace);
}


#ifndef VCL_NO_DEBUG
#define assert_vcl(X, MSG)   {if( (X)==false ) {vcl::call_error(#X,MSG,__FILE__,__PRETTY_FUNCTION__,__LINE__,vcl::backtrace());} }
#define assert_vcl_no_msg(X) {if( (X)==false ) {vcl::call_error(#X,"",__FILE__,__PRETTY_FUNCTION__,__LINE__,vcl::backtrace());} }
#else
#define assert_vcl(X, MSG)
#define assert_vcl_no_msg(X)
#endif

#define error_vcl(MSG)  {vcl::call_error("",MSG,__FILE__,__PRETTY_FUNCTION__,__LINE__,vcl::backtrace()); }


