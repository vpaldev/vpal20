//------------------------------------------------------------------------------
/*
    This file is part of vpald: https://github.com/vpallabs/vpal
    Copyright (c) 2012, 2013 Vpal Labs Inc.

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

#ifndef VPAL_IDIVIDENDVOTE_H
#define VPAL_IDIVIDENDVOTE_H

namespace ripple {

/** Manager to process dividend votes. */
class DividendVote
{
public:
    /** Create a new dividend vote manager.

        @param targetDividendTime
        @param targetDividendAmmount
        @param journal
    */

    virtual ~DividendVote () { }

    /** Add local dividend preference to validation.

        @param lastClosedLedger
        @param baseValidation
    */
    virtual void doValidation (Ledger::ref lastClosedLedger,
                               STObject& baseValidation) = 0;

    /** Cast our local vote on the dividend.

        @param lastClosedLedger
        @param initialPosition
    */
    virtual void doVoting (Ledger::ref lastClosedLedger,
                           SHAMap::ref initialPosition) = 0;
};

std::unique_ptr<DividendVote>
make_DividendVote (std::uint32_t targetDividendTime, std::uint64_t targetDividendAmount,
    std::uint64_t targetDividendAmountVBC, beast::Journal journal);

}

#endif