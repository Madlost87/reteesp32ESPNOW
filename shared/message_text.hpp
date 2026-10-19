#pragma once
#include <stddef.h>
#include <stdint.h>
namespace cantiere {
constexpr size_t TextCapacity=192, TextCharacters=48;
// UTF-8, no embedded controls, overlong encodings, surrogates or out-of-range codepoints.
inline bool validText(const char* text,size_t n) {
  if(n>TextCapacity)return false;
  size_t i=0,characters=0;
  while(i<n) {
    if(++characters>TextCharacters)return false;
    uint8_t c=uint8_t(text[i++]);uint32_t cp=0,minimum=0;unsigned extra=0;
    if(c<0x80){if(c<32 || c==127)return false;continue;}
    if(c>=0xc2 && c<=0xdf){cp=c&31;extra=1;minimum=0x80;}
    else if(c>=0xe0 && c<=0xef){cp=c&15;extra=2;minimum=0x800;}
    else if(c>=0xf0 && c<=0xf4){cp=c&7;extra=3;minimum=0x10000;}
    else return false;
    if(i+extra>n)return false;
    while(extra--){c=uint8_t(text[i++]);if((c&0xc0)!=0x80)return false;cp=(cp<<6)|(c&63);}
    if(cp<minimum || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff))return false;
  }
  return true;
}
}
