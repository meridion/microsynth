
%%

{ident}         return IDENTIFIER;
0[0-9]          return OCTAL;
0x[:xdigit:]+   return HEX;
[0-9]+          return NUM;

%%

