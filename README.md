# m_roleplay

WHAT IS THIS MODULE?!

As per `m_roleplay.cpp`, it "Provides NPC, NPCA, FSAY, FACTION, and SCENE commands for use by Game Masters doing pen & paper RPGs via IRC. Forked off CyberBotX's m_rpg module, and based on Falerin's m_roleplay module for UnrealIRCd."

## Installation

When adding `m_roleplay` to `Inspircd 2.x`, v2.0.23 as of this writing, you will need to place this in `src/modules` *prior* to compiling to add support for roleplay commands to your ircd.

If an error such as `error: WARNING: could not find header m_cap.h for modules/m_roleplay/m_roleplay.cpp` or something very much like it regarding `m_cap.h` pops up during compile, either move the files in `m_roleplay` out of `m_roleplay` into `include/modules` for Inspircd to resolve this error...

Or, y'know, `cp m_cap.h m_roleplay/` in your shell whilst inside `include/modules` to resolve the issue. But the method of moving everything out of `m_roleplay` into `include/modules` is the recommended method.

### LICENSE

`Namegduf` on `irc.inspircd.org` in `#inspircd` has stated that `m_roleplay` is licensed under the same license that [Inspircd](https://github.com/inspircd/inspircd) is.

Source:

```
Namegduf|Awayish: Consider it as under the same license as Insp 
Whichever that was 
And it's a direct port from the UnrealIRCd one, I think the Charybdis one came from elsewhere. 
```

So, for reference on the license, please see `docs/COPYING` for your inspircd install for further details. However, to comply with the GPLv2, we are including a copy of `COPYING` for reference.
