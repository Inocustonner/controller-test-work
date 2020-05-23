#include <cstdio>
#include <string>
// fp file ptr with write attr
void fencode(FILE* fp, std::string& src);
// fp file ptr with read attr
void fdecode(std::string& dst, FILE* fp);