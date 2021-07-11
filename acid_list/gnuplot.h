#ifndef _GNUPLOT_H_
#define _GNUPLOT_H_

#include <cstdio>
#include <string>
#include <iostream>

#define GNUPLOT_NAME "C:\\PROGRA~1\\gnuplot\\bin\\gnuplot.exe -persist"

using std::string;
using std::cerr;

class Gnuplot {
public:
    Gnuplot();
    ~Gnuplot();
    void operator ()(const string & command);

protected:
    FILE *gnuplotpipe;
};

Gnuplot::Gnuplot() {

    this->gnuplotpipe = _popen(GNUPLOT_NAME, "w");
    if (!this->gnuplotpipe) cerr << ("Gnuplot not found !");
}
Gnuplot::~Gnuplot() {
    fprintf(this->gnuplotpipe, "exit\n");
    _pclose(gnuplotpipe);
}

void Gnuplot::operator()(const string &command) {
    fprintf(gnuplotpipe, "%s\n", command.c_str());
    fflush(gnuplotpipe);
};

#endif