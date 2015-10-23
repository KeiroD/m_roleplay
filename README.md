# m_roleplay

WHAT IS THIS MODULE?!

As per `m_roleplay.cpp`, it "Provides NPC, NPCA, FSAY, FACTION, and SCENE commands for use by Game Masters doing pen & paper RPGs via IRC. Forked off CyberBotX's m_rpg module, and based on Falerin's m_roleplay module for UnrealIRCd."

## Installation

When adding `m_roleplay` to `Inspircd 2.x`, v2.0.20 as of this writing, you will need to place this in `src/modules` *prior* to compiling support for roleplay commands.

If an error such as `error: WARNING: could not find header m_cap.h for modules/m_roleplay/m_roleplay.cpp` pops up during compile, either move the files in `m_roleplay` out of `m_roleplay` into `src/modules` for Inspircd to resolve this error.

Or, y'know, `cp m_cap.h m_roleplay/` in your shell to resolve the issue. But the method of moving everything out of `m_roleplay` into `src/modules` is the recommended method.

### LICENSE

No freaking clue. No license came with this when I got it from Namedguf over on irc.inspircd.org. I can't remember who actually told me that he had the Inspircd port.

I suppose you should probably assume it's GPL. Probably GPLv2, come to think of it.

If we check `m_roleplay.cpp`, we see that it's actually licensed but no `COPYING` file was provided with this.

```
/* $ModDesc: Provides NPC, NPCA, FSAY, FACTION, and SCENE commands for use
 * by Game Masters doing pen & paper RPGs via IRC. Forked off CyberBotX's 
 * m_rpg module, and based on Falerin's m_roleplay module for UnrealIRCd. */
/* $ModAuthor: John Robert Beshir (Namegduf) */
/* $ModAuthorMail: namegduf@tellaerad.net */
/* Tributes also go to aquanight for the callback system, in addition to those
 * to the two named above. */
 ```
