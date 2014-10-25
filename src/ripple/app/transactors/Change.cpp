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

namespace ripple {

class Change
    : public Transactor
{
public:
    Change (
        SerializedTransaction const& txn,
        TransactionEngineParams params,
        TransactionEngine* engine)
        : Transactor (
            txn,
            params,
            engine,
            deprecatedLogs().journal("Change"))
    {
    }

    TER doApply () override
    {
        if (mTxn.getTxnType () == ttAMENDMENT)
            return applyAmendment ();

        if (mTxn.getTxnType () == ttFEE)
            return applyFee ();

		if (mTxn.getTxnType() == ttDIVIDEND)
			return applyDividend();

        return temUNKNOWN;
    }

    TER checkSig () override
    {
        if (mTxn.getFieldAccount160 (sfAccount).isNonZero ())
        {
            m_journal.warning << "Bad source account";
            return temBAD_SRC_ACCOUNT;
        }

        if (!mTxn.getSigningPubKey ().empty () || !mTxn.getSignature ().empty ())
        {
            m_journal.warning << "Bad signature";
            return temBAD_SIGNATURE;
        }

        return tesSUCCESS;
    }

    TER checkSeq () override
    {
        if ((mTxn.getSequence () != 0) || mTxn.isFieldPresent (sfPreviousTxnID))
        {
            m_journal.warning << "Bad sequence";
            return temBAD_SEQUENCE;
        }

        return tesSUCCESS;
    }

    TER payFee () override
    {
        if (mTxn.getTransactionFee () != STAmount ())
        {
            m_journal.warning << "Non-zero fee";
            return temBAD_FEE;
        }

        return tesSUCCESS;
    }

    TER preCheck () override
    {
        mTxnAccountID = mTxn.getSourceAccount ().getAccountID ();

        if (mTxnAccountID.isNonZero ())
        {
            m_journal.warning << "Bad source id";

            return temBAD_SRC_ACCOUNT;
        }

        if (mParams & tapOPEN_LEDGER)
        {
            m_journal.warning << "Change transaction against open ledger";
            return temINVALID;
        }

        return tesSUCCESS;
    }

private:
    TER applyAmendment ()
    {
        uint256 amendment (mTxn.getFieldH256 (sfAmendment));

        SLE::pointer amendmentObject (mEngine->entryCache (
            ltAMENDMENTS, Ledger::getLedgerAmendmentIndex ()));

        if (!amendmentObject)
        {
            amendmentObject = mEngine->entryCreate(
                ltAMENDMENTS, Ledger::getLedgerAmendmentIndex());
        }

        STVector256 amendments (amendmentObject->getFieldV256 (sfAmendments));

        if (std::find (amendments.begin(), amendments.end(),
            amendment) != amendments.end ())
        {
            return tefALREADY;
        }

        amendments.push_back (amendment);
        amendmentObject->setFieldV256 (sfAmendments, amendments);
        mEngine->entryModify (amendmentObject);

        getApp().getAmendmentTable ().enable (amendment);

        if (!getApp().getAmendmentTable ().isSupported (amendment))
            getApp().getOPs ().setAmendmentBlocked ();

        return tesSUCCESS;
    }

    TER applyFee ()
    {
        SLE::pointer feeObject = mEngine->entryCache (
            ltFEE_SETTINGS, Ledger::getLedgerFeeIndex ());

        if (!feeObject)
            feeObject = mEngine->entryCreate (
                ltFEE_SETTINGS, Ledger::getLedgerFeeIndex ());

        m_journal.trace << 
            "Previous fee object: " << feeObject->getJson (0);

        feeObject->setFieldU64 (
            sfBaseFee, mTxn.getFieldU64 (sfBaseFee));
        feeObject->setFieldU32 (
            sfReferenceFeeUnits, mTxn.getFieldU32 (sfReferenceFeeUnits));
        feeObject->setFieldU32 (
            sfReserveBase, mTxn.getFieldU32 (sfReserveBase));
        feeObject->setFieldU32 (
            sfReserveIncrement, mTxn.getFieldU32 (sfReserveIncrement));

        mEngine->entryModify (feeObject);

        m_journal.trace << 
            "New fee object: " << feeObject->getJson (0);
        m_journal.warning << "Fees have been changed";
        return tesSUCCESS;
    }

	class pair_less {
        public:
            bool operator()(const std::pair<RippleAddress, uint64_t> &x, const std::pair<RippleAddress, uint64_t> &y)
            {
                    return x.second < y.second;
            }
	};

	TER applyDividend()
	{
		SLE::pointer dividendObject = mEngine->entryCache(
			ltDIVIDEND, Ledger::getLedgerDividendIndex());

		if (!dividendObject) {
			dividendObject = mEngine->entryCreate(
				ltDIVIDEND, Ledger::getLedgerDividendIndex());
		}

		m_journal.info <<
			"Previous dividend object: " << dividendObject->getJson(0);
		uint64_t totalCoins = mTxn.getFieldU64(sfTotalCoins);
		uint64_t totalCoinsVBC = mTxn.getFieldU64(sfTotalCoinsVBC);

		uint64_t dividendAmount = totalCoins * 0.003;
		// REMARK - Please Review VBC's Increase Rate
		uint64_t dividendAmountVBC = totalCoinsVBC * 0.0003;

		dividendObject->setFieldU32(sfDividendTime, getApp().getOPs().getNetworkTimeNC()/SECONDS_PER_DAY);
		dividendObject->setFieldU64(sfTotalCoins, totalCoins+dividendAmount);
		dividendObject->setFieldU64(sfTotalCoinsVBC, totalCoinsVBC + dividendAmountVBC);

		mEngine->entryModify(dividendObject);

		m_journal.info <<
			"Current dividend object: " << dividendObject->getJson(0);

		std::vector<std::pair<RippleAddress, uint64_t> > accounts;
		mEngine->getLedger()->visitStateItems(std::bind(retrieveAccount,
            std::ref(accounts), std::placeholders::_1));
		std::sort(accounts.begin(), accounts.end(), pair_less());
		hash_map<RippleAddress, uint32_t> rank;
		int r = 1;
		rank[accounts[0].first] = r;
		int sum = r;
		for (int i=1; i<accounts.size(); ++i) {
			if (accounts[i].second > accounts[i-1].second)
				++r;
			rank[accounts[i].first] = r;
			sum += r;
		}
		std::for_each(rank.begin(), rank.end(), dividend_account(mEngine, dividendAmount, sum, m_journal));

		return tesSUCCESS;
	}

	class dividend_account {
		RippleAddress root;
		TransactionEngine *engine;
		uint64_t totalDividend;
		uint32_t totalPart;
		beast::Journal m_journal;
	public:
		dividend_account(TransactionEngine *e, uint64_t d, uint32_t p, const beast::Journal &j)
			: engine(e), totalDividend(d), totalPart(p), m_journal(j)
		{
            RippleAddress rootSeedMaster = RippleAddress::createSeedGeneric ("masterpassphrase");
            RippleAddress rootGeneratorMaster = RippleAddress::createGeneratorPublic (rootSeedMaster);
            root = RippleAddress::createAccountPublic (rootGeneratorMaster, 0);
		}

		void operator ()(const std::pair<RippleAddress, uint64_t> &v)
		{
			uint64_t div = totalDividend * v.second / totalPart;
			m_journal.info << v.first.humanAccountID() << "\t" << root.humanAccountID();
			if (div>0 && v.first!=root) {
				auto const index = Ledger::getAccountRootIndex (v.first);
				SLE::pointer sleDst (engine->entryCache (ltACCOUNT_ROOT, index));
				if (sleDst) {
					engine->entryModify(sleDst);
					uint64_t prevBalance = sleDst->getFieldAmount(sfBalance).getNValue();
					sleDst->setFieldAmount(sfBalance, prevBalance+div);
				}
			}
		}
	};

	static void retrieveAccount (std::vector<std::pair<RippleAddress, uint64_t> > &accounts, SLE::ref sle)
	{
		if (sle->getType() == ltACCOUNT_ROOT) {
			RippleAddress addr = sle->getFieldAccount(sfAccount);
			uint64_t bal = sle->getFieldAmount(sfBalance).getNValue();
			accounts.push_back(std::make_pair(addr, bal));
		}
	}

    // VFALCO TODO Can this be removed?
    bool mustHaveValidAccount () override
    {
        return false;
    }
};


TER
transact_Change (
    SerializedTransaction const& txn,
    TransactionEngineParams params,
    TransactionEngine* engine)
{
    return Change (txn, params, engine).apply ();
}

}
