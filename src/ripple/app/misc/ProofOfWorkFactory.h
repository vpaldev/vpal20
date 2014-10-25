//------------------------------------------------------------------------------
/*
    Portions of this file are from Vpallab: https://github.com/vpallabs
    Copyright (c) 2013 - 2014 - Vpallab.com.
    Please visit http://www.vpallab.com/
    
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_PROOFOFWORKFACTORY_H_INCLUDED
#define RIPPLE_PROOFOFWORKFACTORY_H_INCLUDED

#include <ripple/app/misc/PowResult.h>
#include <ripple/app/misc/ProofOfWork.h>
#include <beast/cxx14/memory.h> // <memory>

namespace ripple {

class ProofOfWorkFactory
{
public:
    enum
    {
        kMaxDifficulty = 30,
    };

    virtual ~ProofOfWorkFactory () { }

    virtual ProofOfWork getProof () = 0;

    virtual PowResult checkProof (std::string const& token, uint256 const& solution) = 0;

    virtual std::uint64_t getDifficulty () = 0;

    virtual void setDifficulty (int i) = 0;

    virtual void loadHigh () = 0;

    virtual void loadLow () = 0;

    virtual void sweep () = 0;

    virtual uint256 const& getSecret () const = 0;

    virtual void setSecret (uint256 const& secret) = 0;
};

std::unique_ptr<ProofOfWorkFactory> make_ProofOfWorkFactory ();

}

#endif