#include "moto-protocol.h"
#include "motogame.h"
#include "main.h"
#ifdef _WIN32
    #include <windows.h>
#endif

static std::unique_ptr<Motogame> g_pMotogame;

static QString getMotogame(bool LowQ, bool OGL3)
{
    QString Prefix = "./";
#ifdef _WIN32
    if (!OGL3)
    {
        SYSTEM_INFO SysInfo;
        GetNativeSystemInfo(&SysInfo);
        if (SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
            Prefix = "./llvmpipe/64/";
        else
            Prefix = "./llvmpipe/32/";
    }
#endif
    return Prefix + "motogame -nofun" + (LowQ? " -schematic" : "");
}

Motogame::Motogame(bool LowQ, bool OGL3, CWallet* pWallet, QObject *parent) :
    QObject(parent), m_Motogame(this), m_pWallet(pWallet), m_ReserveKey(pWallet), m_pPrevBest(nullptr)
{
    m_Motogame.start(getMotogame(LowQ, OGL3));

    connect(&m_Motogame, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadAvailable()));
    connect(&m_Motogame, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onGameFinished(int, QProcess::ExitStatus)));
    connect(&m_Motogame, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onError(QProcess::ProcessError)));

    updateBlock();
    startTimer(500);
}

Motogame::~Motogame()
{
    m_Motogame.kill();
    g_pMotogame.release();
}

void Motogame::onGameFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    deleteLater();
}

void Motogame::onError(QProcess::ProcessError error)
{
    deleteLater();
}

void Motogame::timerEvent(QTimerEvent*)
{
    updateBlock();
}

std::list<std::unique_ptr<CBlockTemplate>>::iterator Motogame::findBlock(const MotoWork& Work)
{
    for (auto iter = m_Templates.begin(); iter != m_Templates.end(); ++iter)
    {
        if (memcmp(&(*iter)->block.nVersion, Work.Block, MOTO_WORK_SIZE) == 0)
            return iter;
    }
    return m_Templates.end();
}

void Motogame::onReadAvailable()
{
    while (true)
    {
        QByteArray Msg = m_Motogame.readLine();
        if (Msg.size() == 0)
            break;

        MotoWork Work;
        MotoPoW PoW;
        if (motoParseMessage(Msg.data(), Work))
        {
            auto iter = findBlock(Work);
            if (iter != m_Templates.end())
                m_Templates.erase(iter);
        }
        else if (motoParseMessage(Msg.data(), Work, PoW))
        {
            auto iter = findBlock(Work);
            if (iter != m_Templates.end())
            {
                CBlock *pBlock = &(*iter)->block;
                pBlock->Nonce = PoW;
                CheckWork(pBlock, *m_pWallet, m_ReserveKey);
            }
            updateBlock();
        }
    }
}

void Motogame::updateBlock()
{
    // Create new block.
    std::unique_ptr<CBlockTemplate> pBlockTemplate(CreateNewBlockWithKey(m_ReserveKey));
    if (!pBlockTemplate.get())
        return;
    CBlock *pBlock = &pBlockTemplate->block;
    unsigned int nExtraNonce = 0;
    IncrementExtraNonce(pBlock, pindexBest, nExtraNonce);

    // Check if block has changed.
   /* CBlock *pLastBlock = m_Templates.empty()? nullptr : &m_Templates.back()->block;
    if (pLastBlock &&
        pBlock->hashPrevBlock == pLastBlock->hashPrevBlock &&
        pBlock->hashMerkleRoot == pLastBlock->hashMerkleRoot &&
        pBlock->nTime - pLastBlock->nTime < 30) // Update nTime every 30 sec
        return;*/

   // printf("Running MotocoinMiner with %" PRIszu " transactions in block (%u bytes)\n", pBlock->vtx.size(),
   //        ::GetSerializeSize(*pBlock, SER_NETWORK, PROTOCOL_VERSION));

    bool IsNew = m_pPrevBest != pindexBest;
    m_pPrevBest = pindexBest;
    if (IsNew)
        m_Templates.clear();

    if (m_Templates.size() > 16) // This should not happen.
        m_Templates.pop_front();

    // Save all block templates. We may need older template because player may solve not the last block.
    m_Templates.push_back(std::move(pBlockTemplate));

    // Construct work.
    MotoWork Work;
    Work.IsNew = IsNew;
    Work.TimeTarget = pBlock->nBits;
    snprintf(Work.Msg, sizeof(Work.Msg), "Block %i, Reward %f MOTO, Target %.3f.", pindexBest->nHeight + 1, pBlock->vtx[0].vout[0].nValue/100000000.0, Work.TimeTarget/250.0);
    memcpy(Work.Block, &pBlock->nVersion, sizeof(Work.Block));

    // Send work message to motogame.
    std::string Msg = motoMessage(Work);
    m_Motogame.write(Msg.c_str());
}

void startMining(bool LowQ, bool OGL3, CWallet* pWallet)
{
    if (!g_pMotogame)
        g_pMotogame.reset(new Motogame(LowQ, OGL3, pWallet));
}

void watchReplay(bool LowQ, bool OGL3, int nHeight)
{
    if (nHeight <= 0 || nHeight > nBestHeight)
        return;

    CBlockIndex* pIndex = FindBlockByHeight(nHeight);
    if (!pIndex)
        return;

    struct
    {
        int nVersion;
        uint256 hashPrevBlock;
        uint256 hashMerkleRoot;
        unsigned int nTime;
        unsigned int nBits;
        MotoPoW Nonce;
    } BlockHeader;

    BlockHeader.nVersion = pIndex->nVersion;
    BlockHeader.hashPrevBlock = pIndex->pprev? pIndex->pprev->GetBlockHash() : 0;
    BlockHeader.hashMerkleRoot = pIndex->hashMerkleRoot;
    BlockHeader.nTime = pIndex->nTime;
    BlockHeader.nBits = pIndex->nBits;
    BlockHeader.Nonce = pIndex->Nonce;

    MotoWork Work;
    Work.IsNew = true;
    Work.TimeTarget = pIndex->nBits;
    snprintf(Work.Msg, sizeof(Work.Msg), "Block %i.", nHeight);
    memcpy(Work.Block, &BlockHeader, sizeof(Work.Block));

    std::string Msg = motoMessage(Work, pIndex->Nonce);

    SuicideProcess* pMotogameProcess = new SuicideProcess;
    pMotogameProcess->start(getMotogame(LowQ, OGL3));
    pMotogameProcess->write(Msg.c_str());
}
