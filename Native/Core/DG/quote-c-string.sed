s/\\/\\\\/g
s/"/\\"/g
s/^\(.*\)$/"\1\\n"/
s/function/"\)+std::string\("function/g
1i\
std::string(
$a\
)
