#pragma once
/* this macro is for conditionally compiling code for
   debug builds. The typical faclities to enable this
   functionality are "#ifdef"s.

       #ifdef XXX_DEBUG
           // ... debug only code ...
       #else
           // ... release only code ...
       #endif

    But those two lines are quite code pollutive and add a lot of visual clutter.
    Code that you'd only like to run in debug/dev builds are VERY common. There
    are a lot of runtime checks for validation that you'd rather only do in debug
    builds for assurence of valid logic and "debugability", but in release, you'd
    rather proritize speed, because if something is broken, you can't fix it now,
    its in the consumers hands, might as well not let their peformance be hindered
    by the checks just in case nothing goes wrong.
*/

#ifndef DBG_NO_DBG
/* @note: this could also be renamed to `if_dbg`. The name would be more unique.
          and is a little more descriptive, "if dbg (debugging) then do X". But
          "dbg" also works, treat it "as debug code { is here }". The problem also is
          that is an verb, debug THIS, it's not very descriptive as a scope of code
          that is ran during debug -- OH GOD, I'm in naming hell! 😭
*/
#define dbg  if constexpr (true) 
// you could just do `if_dbg` without the "()", but... I have my reasons.
#define if_dbg() if constexpr(true)
#define dbg_if(...) if (( __VA_ARGS__ ))
#else
#define dbg if constexpr (false)
#define if_dbg() if constexpr(false)
#define dbg_if(...) if constexpr (false)
#endif

/* the reason this macro is all lowercase is beacuse
   its intended to be treated as a keyword.
   same as `defer` in defer.hxx. */

/* e.g.
    dbg {
        if (X == Y)
        {
            printf("this will only be exeucted in "debug" builds!\n");
        }
    }

// or:

    dbg_if(X == Y)
    {
        printf("this will only be exeucted in "debug" builds!\n");
    }

// or: 
    dbg if (X == Y) 😀
    {
        printf("this will only be exeucted in "debug" builds!\n");
    }
            (be careful with this one, if you append an `else` 
             branch it will also be ridded of in non debug builds.)
*/

/* I also wouldn't mind

    if_dbg()
    {
        some code
    }

    if_dbg_if(1 == 2)
    {
        some code
    }

 It's symmetrical and has a obvious pattern that programmer could follow.
 there are no other keywords in the langauge that can be followed be immediate
 curly braces apart from unnamed structs/unions. Having the braces around it
 makes it look more like it's executing code rather than... whatever `dbg` would do.
*/