// =====================================================================================
//
//       Filename:  IAF.h
//
//    Description:  Integrate and fire neuron.
//
//        Version:  1.0
//        Created:  Saturday 27 April 2019 04:52:03  IST
//       Revision:  none
//       Compiler:  g++
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore
//
// =====================================================================================

#ifndef IAF_H
#define IAF_H

#define MAX_SYNAPSES 100

#include <systemc.h>
#include <vector>
#include <tuple>
#include <array>
#include <memory>
#include <random>

#include "SynapseExp.h"
#include "SynapseAlpha.h"

using namespace std;

class IAF : public sc_module
{
public:
    SC_HAS_PROCESS(IAF);


    //-----------------------------------------------------------------------------
    //  Constructors.
    //-----------------------------------------------------------------------------
    IAF(sc_module_name path, double tau=10e-3, double em=-65e-3);
    IAF(sc_module_name path, double cm, double rm, double em);

    void init();

    //-----------------------------------------------------------------------------
    //  Ports
    //-----------------------------------------------------------------------------
    sc_in_clk clock{"clock"};                       // Incoming clock.
    sc_out<double> vm{"vm"};                        // Neuron membrane potential.

    // Let make it optional.
    sc_in<double> inject{"inject"};                 // Injection of current by users.

    // sc_vector<sc_in<double>> psc;                   // post synaptic current.
    std::vector<unique_ptr<sc_in<double>>> psc;      // post synaptic current.

    // Deferred binding
    virtual void before_end_of_elaboration();

    //-----------------------------------------------------------------------------
    //  Events.
    //-----------------------------------------------------------------------------
    sc_event nonZeroInject;                 // If non-zero injection of current.
    sc_event nonZeroSynCurrent;             // If non-zero injection via synapses.
    sc_event onFire;                        // If this neuron has fired.

    void decay();
    void record();
    void handleInjection();                 // Tick when there is inject
    void handleSynapticInjection();
    void handleOnFire();

    void model(const double &vm, double &dvdt, const double t);

    // Add synapse.
    void bindSynapse(sc_signal<double>* sig);

    //-----------------------------------------------------------------------------
    //  Mutators.
    //-----------------------------------------------------------------------------
    void setRefactory(double t);
    void setTau(double t);
    void setNoise(double eps);
    void setThreshold(double t);

    //-----------------------------------------------------------------------------
    //  Accessor.
    //-----------------------------------------------------------------------------
    std::vector<std::tuple<double, double>> data( ) const;
    string path() const;

    //-----------------------------------------------------------------------------
    //  Helper
    //-----------------------------------------------------------------------------
    string repr();
    double noise();

    void save_data(const string& outfile="");

private:
    // Collect synapses.
    string path_;

    std::vector<std::tuple<double, double>> data_;
    std::vector<double> spikes_;            // Time of spikes.

    double Cm_;
    double Em_;
    double Rm_;
    double vm_;
    double tau_;
    double threshold_;
    double refactory_;
    bool fired_;

    // Helper variables.
    double t_, prevT_;
    double dt_;
    double sum_all_synapse_inject_;

    size_t numSynapses_ {0};

    // noise source.
    std::default_random_engine gen_;
    std::uniform_real_distribution<double>  dist_{-1, 1};
    double noise_;

    // temporary signals to save the incoming psc binding (if any).
    vector<sc_signal<double>*> psc_temp_;
};

#endif /* end of include guard: IAF_H */
