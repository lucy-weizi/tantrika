/***
 *    Description:  Network.
 *
 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 *        License:  MIT License
 */


#include "../include/Network.h"
#include "../include/SynapseGroup.h"
#include "../include/NeuronGroup.h"
#include "../utility/data_util.h"

#include "NetworkConnectors.hpp"
#include "NetworkShowInfoVisitor.hpp"

// Global 
sc_time globalDt_;

Network::Network(sc_module_name name, double dt): 
    name_(name)
    , dt_(sc_time(dt, SC_SEC))
{ 

#ifdef LOG_LEVEL
    spdlog::set_level(spdlog::level::LOG_LEVEL);
#endif

    globalDt_ = dt_;

    path_ = string((const char*)name_);
    spdlog::info("Creating network: {}", path_ );

    // setup clock.
    clock_ = make_unique<sc_clock>("clock", dt_, SC_SEC);
    spdlog::info("Global clock dt {} s", clock_->period().to_seconds() );

    SC_METHOD(record);
    sensitive << clock_->posedge_event();
}


/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Record all signals.
 */
/* ----------------------------------------------------------------------------*/
void Network::record()
{
    // Put data into vectors.
    data_["time"].push_back(sc_time_stamp().to_seconds());
    for(auto& s: signals_)
        data_[s.second->name()].push_back(s.second->read());
    for(auto& s: boolSignals_)
        data_[s.second->name()].push_back(s.second->read());
}

void Network::before_end_of_elaboration()
{
    std::vector<network_variant_t> elems;
    findElementsByType("NeuronGroup", elems);
    for(auto& e: elems)
    {
        NeuronGroup* nrns = boost::get<NeuronGroup*>(e);
        for (size_t i = 0; i < nrns->size(); i++) 
            nrns->getNeuron(i)->before_end_of_elaboration();
    }

    bindUnboundPorts();

    // And end_of_elaboration will take care of the rest.
}


void Network::showGroupInfo(const string& path)
{
    auto elem = findGroup(path);
    boost::apply_visitor(NetworkShowInfoVisitor(), elem);
}


void Network::saveData(const string& which, const string& sep)
{
    map2csv(data_, path()+".csv", sep);
}

// Add synapse groups.
void Network::addSynapseGroup(const string& path, size_t N
        , double gbar, double tau, double Esyn
        , const string type
        )
{
    SynapseGroup* syn = new SynapseGroup(path.c_str(), N, gbar, tau, Esyn, type);
    addToMaps<SynapseGroup>("SynapseGroup",  syn);
    spdlog::info("Created SynapseGroup: {} of size {}.", path, N);

    // bind ports.
    string sigName;
    for (size_t i = 0; i < syn->size(); i++) 
    {
        SynapseBase* s = syn->getSynapse(i);
        // bind clock.
        s->clock(*clock_);
    }
}

// Neuron group.
void Network::addNeuronGroup(const string& path, size_t N, double rm, double cm, double Em)
{

    NeuronGroup* ng = new NeuronGroup(path.c_str(), N, rm, cm, Em);
    addToMaps<NeuronGroup>("NeuronGroup", ng);

   // Now bind ports.
    string sigName;
    for (size_t i = 0; i < N; i++) 
    {
        IAF* n = ng->getNeuron(i);
        n->clock(*clock_);
    }
}

void Network::addSignal(unique_ptr<sc_signal<bool>> sig)
{
    boolSignals_[sig->name()] = std::move(sig);
}

void Network::addSignal(unique_ptr<sc_signal<double>> sig)
{
    signals_[sig->name()] = std::move(sig);
}

// Add spike generator, Poisson Group.
void Network::addPoissonGroup(const string& path, size_t N, double lambda)
{
    spdlog::error( "Network::addSpikeGeneratorGroup is not implemented yet." );
    throw TantrikaNotImplemented();
}

// Add a spikegenerator group.
void Network::addSpikeGeneratorGroup(const string& path, size_t N, const string type, ...)
{
    spdlog::error( "Network::addSpikeGeneratorGroup is not implemented yet." );
    throw TantrikaNotImplemented();
}

void Network::addSpikeGeneratorPeriodicGroup(const string& path, size_t N, double period, double delay)
{
    SpikeGeneratorBase* spikeGen_ = new PeriodicSpikeGenerator(path.c_str(), N, period);
    spikeGen_->setDelay(delay);

    addToMaps<SpikeGeneratorBase>("SpikeGeneratorPeriodicGroup", spikeGen_);
    spdlog::info("Created SpikeGeneratorGroup {} with size {}", path, N);

    // Bind ports.
    bindPortSpikeGeneratorBase(spikeGen_);
}

void Network::bindPortSpikeGeneratorBase(SpikeGeneratorBase* spk)
{
    spk->clock(*clock_);
}

int Network::bindPorts(network_variant_t elem)
{
    return boost::apply_visitor(std::bind(NetworkPortBinderVisitor(), std::placeholders::_1, this), elem);
}


/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Bind all the ports with local signal which has not been bound by
 * user.
 */
/* ----------------------------------------------------------------------------*/
void Network::bindUnboundPorts(void)
{
    spdlog::info( "Connecting unbound ports .. " );
    for(auto& v : elemMap_)
        bindPorts(v.second);
}


string Network::path() const
{
    return path_;
}

network_variant_t Network::findGroup(const string& path)
{
    return elemMap_.find(path)->second; 
}

// Find by type.
void Network::findElementsByType(const string& type, std::vector<network_variant_t>& collect)
{
    auto range = typeMap_.equal_range(type); 
    for( auto i = range.first; i != range.second; i++)
        collect.push_back(i->second);
}

// Connect port.
int Network::connect(const string& srcPath, const string& srcPort
        , const string& tgtPath , const string& tgtPort)
{
    spdlog::info("Connecting {}.{} --> {}.{} ...", srcPath, srcPort, tgtPath, tgtPort);
    auto src = findGroup(srcPath);
    auto tgt = findGroup(tgtPath);
    return boost::apply_visitor(
            std::bind(NetworkConnectionVisitor(), std::placeholders::_1, srcPort, tgt, tgtPort, this)
            , src);
}
 



/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  sc_main/main function. This function is called when user simulate
 * system.
 *
 * @Param runtime
 *
 * @Returns   
 */
/* ----------------------------------------------------------------------------*/
int Network::start(double runtime)
{

    auto t1 = std::chrono::high_resolution_clock::now();

    // Turn them to US.
    sc_start(runtime, SC_SEC);
    std::chrono::duration<double> timeTaken = std::chrono::high_resolution_clock::now() - t1;
    spdlog::info("Simulation over. Took {}s to simulate {}s.", timeTaken.count(), runtime);
    // dumpData();

#if 0
    try
    {
        sc_start(runtime, SC_SEC);
    } 
    catch(std::exception& e )
    {

        cerr << "Failed to run." << endl;
        cerr << e.what() << endl;
        
        throw e;
    }
#endif 

    return 0;
}

