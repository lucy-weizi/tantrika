/***
 *    Description:  Helper class
 *
 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 *        License:  MIT License
 */

#include "../include/Synapse.h"

Synapse::Synapse()
{}

Synapse::~Synapse()
{}

Synapse::Synapse(const string path, double gbar, double tau, double Em, const string type)
{
    SynapseBase* baseSyn = nullptr;
    if(type == "alpha")
        baseSyn = new SynapseAlpha(path.c_str(), gbar, tau, Em);
    else
        baseSyn = new SynapseExp(path.c_str(), gbar, tau, Em);

    // Assing to unique ptr.
    assert(baseSyn);
    syn_.reset(baseSyn);
    
}