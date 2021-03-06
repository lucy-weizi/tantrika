/*
 *    Description:  Plot utility.
 *
 *         Author:  Dilawar Singh (), dilawars@ncbs.res.in
 *   Organization:  NCBS Bangalore
 */

#include "data_util.h"

#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/combine.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/log/trivial.hpp>

#include <iostream>

using namespace std;
namespace io = boost::iostreams;

void plot_vectors(const vector<double>& t, const vector<double>& y)
{
    std::cout << "Plotting vector of size: " << y.size() << std::endl;
    Gnuplot gp;
    gp.send1d(boost::make_tuple(t, y));
}

void plot_data(const vector<std::tuple<double, double>>& data
        , const string& title
        , const string& xlabel
        , const string& ylabel)
{
    std::cout << "Plotting vector of size: " << data.size() << std::endl;
    Gnuplot gp;
    gp << "set xlabel '" << xlabel << "';set ylabel '" << ylabel << "';";
    gp << "plot '-' with lp title '" << title << "' ;";
    gp.send1d(data);
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Write a map<string, vector<double>> to csv file.
 *
 * @Param data
 * @Param csvname
 * @Param delim
 */
/* ----------------------------------------------------------------------------*/
void map2csv( const map<string, vector<double>>& data, const string& csvname, const char delim)
{
    std::string delimStr(1, delim); // for boost join.

    io::stream_buffer<io::file_sink> buf(csvname);
    std::ostream csvF(&buf);

    vector<string> header;
    size_t nVals = 10000000;

    // Write header.
    for(auto v : data)
    {
        header.push_back(v.first);
        nVals = min(v.second.size(), nVals);
    }

    csvF << boost::algorithm::join(header, delimStr) << endl;

    for (size_t i = 0; i < nVals; i++) 
    {
        for(auto v : data)
            csvF << v.second[i] << delim;
        csvF.seekp(-1, std::ios_base::cur);     // Remove last delim
        csvF << endl;                           
    }
    buf.close();
    BOOST_LOG_TRIVIAL(info) << "Saved " << data.size() << " entries to " << csvname;
}

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Write a vector of tuple to csv file.
 *
 * @Param data      vector<tuple<double, double>> data_
 * @Param outfile   output file. If not given, use name_ + ".csv"
 * @Param header    header to add to csv file.
 */
/* ----------------------------------------------------------------------------*/
void write_to_csv(const vector<tuple<double,double>>& data, const string outfile, const string header)
{
    io::stream_buffer<io::file_sink> buf(outfile);
    std::ostream f(&buf);

    if(header.size() > 0)
        f << header << endl;

    for(auto v : data)
        f << get<0>(v) << ',' << get<1>(v) << endl;

    buf.close();
    BOOST_LOG_TRIVIAL(info) << "Saved " << data.size() << " entries to " << outfile;
}
