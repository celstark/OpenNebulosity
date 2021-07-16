#include "qsiapi.h"
// clang -stdlib=libstdc++ -arch i386 -I ../lib -L../lib/.libs -L/opt/local/lib -lqsiapi -lftd2xx -lc++ -lstdc++ -o cs_test cs_test.cpp

int main() {

QSICamera *pCam;

pCam = new QSICamera;

return 0;

}
