/*----------------------------------------------------------------------*
 * Copyright(c) 2015 SAP SE. All rights reserved
 * Author      : SAP Custom Development
 * Description : Utility for auto clean-up at scope exit
 *----------------------------------------------------------------------*
 * Change - History : Change history
 * Developer  Date      Description
 * I078212    20150227  Initial creation
 *----------------------------------------------------------------------*/

#ifndef __AT_SCOPE_EXIT_H__
#define __AT_SCOPE_EXIT_H__

template <class Lambda>
class AtScopeExit
{
public:
    AtScopeExit(Lambda& action) : lambda_(action)
    {}
    ~AtScopeExit()
    {
        lambda_();
    }
private:
    Lambda& lambda_;
};

#define TOKEN_PASTEx(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN_PASTEx(x, y)

#define Auto_INTERNAL1(lname, aname, ...) \
    auto lname = [&]() {__VA_ARGS__;}; \
    AtScopeExit<decltype(lname)> aname(lname);

#define Auto_INTERNAL2(ctr, ...) \
    Auto_INTERNAL1(TOKEN_PASTE(Auto_func_, ctr), \
                   TOKEN_PASTE(Auto_instance_, ctr), __VA_ARGS__)

#define AT_SCOPE_EXIT(...) Auto_INTERNAL2(__COUNTER__, __VA_ARGS__)

#endif // __AT_SCOPE_EXIT_H__
