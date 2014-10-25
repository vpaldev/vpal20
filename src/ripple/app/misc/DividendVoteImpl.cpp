//------------------------------------------------------------------------------
/*
    Portions of this file are from Vpallab: https://github.com/vpallabs
    Copyright (c) 2013 - 2014 - Vpallab.com.
    Please visit http://www.vpallab.com/
    
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

namespace ripple {

class DividendVoteImpl : public DividendVote
{
private:

    template <typename Integer>
    class VotableInteger
    {
    public:
        VotableInteger (Integer current, Integer target)
            : mCurrent (current)
            , mTarget (target)
        {
            // Add our vote
            ++mVoteMap[mTarget];
        }

        bool
        mayVote () const
        {
            // If we love the current setting, we will not vote
            return mCurrent != mTarget;
        }

        void
        addVote (Integer vote)
        {
            ++mVoteMap[vote];
        }

        void
        noVote ()
        {
            addVote (mCurrent);
        }

        Integer
        getVotes ()
        {
            Integer ourVote = mCurrent;
            int weight = 0;

            typedef typename std::map<Integer, int>::value_type mapVType;
            for (auto const& e : mVoteMap)
            {
                // Take most voted value between current and target, inclusive
                if ((e.first <= std::max (mTarget, mCurrent)) &&
                        (e.first >= std::min (mTarget, mCurrent)) &&
                        (e.second > weight))
                {
                    ourVote = e.first;
                    weight = e.second;
                }
            }

            return ourVote;
        }

    private:
        Integer mCurrent;   // The current setting
        Integer mTarget;    // The setting we want
        std::map<Integer, int> mVoteMap;
    };

public:
    DividendVoteImpl (std::uint32_t targetDividendTime, std::uint64_t targetDividendAmount,
             std::uint64_t targetDividendAmountVBC, beast::Journal journal)
        : mTargetDividendTime (targetDividendTime)
		, mTargetDividendAmount(targetDividendAmount)
		, mTargetDividendAmountVBC(targetDividendAmountVBC)
		, m_journal(journal)
    {
    }

    //--------------------------------------------------------------------------

    void
    doValidation (Ledger::ref lastClosedLedger, STObject& baseValidation) override
    {
        if (lastClosedLedger->getDividendTime () != mTargetDividendTime)
        {
            if (m_journal.info) m_journal.info <<
                "Voting for dividend time of " << mTargetDividendTime;

            baseValidation.setFieldU32 (sfDividendTime, mTargetDividendTime);
        }

        if (lastClosedLedger->getTotalCoins() != mTargetDividendAmount)
        {
            if (m_journal.info) m_journal.info <<
                "Voting for dividend VRP of " << mTargetDividendAmount;

            baseValidation.setFieldU64(sfTotalCoins, mTargetDividendAmount);
        }

		if (lastClosedLedger->getTotalCoinsVBC() != mTargetDividendAmountVBC) {
			if (m_journal.info) m_journal.info <<
				"Voting for devidend VBC of" << mTargetDividendAmountVBC;

			baseValidation.setFieldU64(sfTotalCoinsVBC, mTargetDividendAmountVBC);
		}
    }

    //--------------------------------------------------------------------------

    void
    doVoting (Ledger::ref lastClosedLedger, SHAMap::ref initialPosition) override
    {
        // LCL must be flag ledger
        assert ((getApp().getOPs().getNetworkTimeNC()/SECONDS_PER_DAY - lastClosedLedger->getDividendTime()) >= 1);

        VotableInteger<std::uint32_t> dividendTimeVote (lastClosedLedger->getDividendTime(), mTargetDividendTime);
		VotableInteger<std::uint64_t> totalCoinsVote(lastClosedLedger->getTotalCoins(), mTargetDividendAmount);
		VotableInteger<std::uint64_t> totalCoinsVBCVote(lastClosedLedger->getTotalCoinsVBC(), mTargetDividendAmountVBC);

        // get validations for ledger before flag
        ValidationSet set = getApp().getValidations ().getValidations (lastClosedLedger->getParentHash ());
        for (auto const& e : set)
        {
            SerializedValidation const& val = *e.second;

            if (val.isTrusted ())
            {
                if (val.isFieldPresent (sfDividendTime))
                {
                    dividendTimeVote.addVote (val.getFieldU32 (sfDividendTime));
                }
                else
                {
                    dividendTimeVote.noVote ();
                }

                if (val.isFieldPresent (sfTotalCoins))
                {
                    totalCoinsVote.addVote (val.getFieldU64 (sfTotalCoins));
                }
                else
                {
                    totalCoinsVote.noVote ();
                }

				if (val.isFieldPresent(sfTotalCoinsVBC))
				{
					totalCoinsVBCVote.addVote(val.getFieldU64(sfTotalCoinsVBC));
				}
				else
				{
					totalCoinsVBCVote.noVote();
				}
			}
        }

        // choose our positions
		mTargetDividendTime = dividendTimeVote.getVotes();
		mTargetDividendAmount = totalCoinsVote.getVotes();
		mTargetDividendAmountVBC = totalCoinsVBCVote.getVotes();

        // add transactions to our position
		uint32_t now = getApp().getOPs().getNetworkTimeNC() / SECONDS_PER_DAY;
		if ((now - mTargetDividendTime) >= 1)
        {
            if (m_journal.warning) m_journal.warning <<
                "We are voting for a dividend: " << now <<
				"/" << mTargetDividendAmount * 0.003;

            SerializedTransaction trans (ttDIVIDEND);
            trans.setFieldAccount (sfAccount, Account ());
			trans.setFieldU32(sfDividendTime, mTargetDividendTime);
			trans.setFieldU64(sfTotalCoins, mTargetDividendAmount);

            uint256 txID = trans.getTransactionID ();

            if (m_journal.warning)
                m_journal.warning << "Vote: " << txID;

            Serializer s;
            trans.add (s, true);

            SHAMapItem::pointer tItem = std::make_shared<SHAMapItem> (txID, s.peekData ());

            if (!initialPosition->addGiveItem (tItem, true, false))
            {
                if (m_journal.warning) m_journal.warning <<
                    "Ledger already had dividend";
            }
        }
    }

private:
    std::uint32_t mTargetDividendTime;
	std::uint64_t mTargetDividendAmount;
	std::uint64_t mTargetDividendAmountVBC;
	beast::Journal m_journal;
};

//------------------------------------------------------------------------------

std::unique_ptr<DividendVote>
make_DividendVote (std::uint32_t targetDividendTime, std::uint64_t targetDividendAmount,
    std::uint64_t targetDividendAmountVBC, beast::Journal journal)
{
    return std::make_unique<DividendVoteImpl> (targetDividendTime, targetDividendAmount,
		targetDividendAmountVBC, journal);
}

}