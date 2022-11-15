// Archive/Cab/MSZipDecoder.cpp

#include "StdAfx.h"

#include "MSZipDecoder.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace NCab {
namespace NMSZip {

CDecoder::CDecoder(){}

HRESULT CDecoder::Flush()
{
  return m_OutWindowStream.Flush();
}

void CDecoder::ReleaseStreams()
{
  m_OutWindowStream.ReleaseStream();
  m_InBitStream.ReleaseStream();
}

void CDecoder::DeCodeLevelTable(Byte *newLevels, int numLevels)
{
  int i = 0;
  while (i < numLevels)
  {
    UInt32 number = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
    if (number < kTableDirectLevels)
      newLevels[i++] = Byte(number);
    else
    {
      if (number == kTableLevelRepNumber)
      {
        int t = m_InBitStream.ReadBits(2) + 3;
        for (int reps = t; reps > 0 && i < numLevels ; reps--, i++)
          newLevels[i] = newLevels[i - 1];
      }
      else
      {
        int num;
        if (number == kTableLevel0Number)
          num = m_InBitStream.ReadBits(3) + 3;
        else
          num = m_InBitStream.ReadBits(7) + 11;
        for (;num > 0 && i < numLevels; num--)
          newLevels[i++] = 0;
      }
    }
  }
}

void CDecoder::ReadTables(void)
{
  if(m_FinalBlock) // test it
    throw CDecoderException(CDecoderException::kData);

  m_FinalBlock = (m_InBitStream.ReadBits(kFinalBlockFieldSize) == NFinalBlockField::kFinalBlock);

  int blockType = m_InBitStream.ReadBits(kBlockTypeFieldSize);

  switch(blockType)
  {
    case NBlockType::kStored:
      {
        m_StoredMode = true;
        UInt32 currentBitPosition = m_InBitStream.GetBitPosition();
        UInt32 numBitsForAlign = currentBitPosition > 0 ? (8 - currentBitPosition): 0;
        if (numBitsForAlign > 0)
          m_InBitStream.ReadBits(numBitsForAlign);
        m_StoredBlockSize = m_InBitStream.ReadBits(kDeflateStoredBlockLengthFieldSizeSize);
        WORD onesComplementReverse = ~WORD(m_InBitStream.ReadBits(kDeflateStoredBlockLengthFieldSizeSize));
        if (m_StoredBlockSize != onesComplementReverse)
          throw CDecoderException(CDecoderException::kData);
        break;
      }
    case NBlockType::kFixedHuffman:
    case NBlockType::kDynamicHuffman:
      {
        m_StoredMode = false;
        Byte litLenLevels[kStaticMainTableSize];
        Byte distLevels[kStaticDistTableSize];
        if (blockType == NBlockType::kFixedHuffman)
        {
          int i;

          // Leteral / length levels
          for (i = 0; i < 144; i++)
            litLenLevels[i] = 8;
          for (; i < 256; i++)
            litLenLevels[i] = 9;
          for (; i < 280; i++)
            litLenLevels[i] = 7;
          for (; i < 288; i++)          /* make a complete, but wrong code set */
            litLenLevels[i] = 8;
        
          // Distance levels
          for (i = 0; i < kStaticDistTableSize; i++)  // test it: infozip only use kDistTableSize       
            distLevels[i] = 5;
        }
        else // in case when (blockType == kDeflateBlockTypeFixedHuffman)
        {
          int numLitLenLevels = m_InBitStream.ReadBits(kDeflateNumberOfLengthCodesFieldSize) + 
            kDeflateNumberOfLitLenCodesMin;
          int numDistLevels = m_InBitStream.ReadBits(kDeflateNumberOfDistanceCodesFieldSize) + 
            kDeflateNumberOfDistanceCodesMin;
          int numLevelCodes = m_InBitStream.ReadBits(kDeflateNumberOfLevelCodesFieldSize) + 
            kDeflateNumberOfLevelCodesMin;
          
          int numLevels;
          numLevels = kHeapTablesSizesSum;
          
          Byte levelLevels[kLevelTableSize];
          int i;
          for (i = 0; i < kLevelTableSize; i++)
          {
            int position = kCodeLengthAlphabetOrder[i]; 
            if(i < numLevelCodes)
              levelLevels[position] = Byte(m_InBitStream.ReadBits(kDeflateLevelCodeFieldSize));
            else
              levelLevels[position] = 0;
          }
          
          try
          {
            m_LevelDecoder.SetCodeLengths(levelLevels);
          }
          catch(...)
          {
            throw CDecoderException(CDecoderException::kData);
          }
          
          Byte tmpLevels[kStaticMaxTableSize];
          DeCodeLevelTable(tmpLevels, numLitLenLevels + numDistLevels);
          
          memmove(litLenLevels, tmpLevels, numLitLenLevels);
          memset(litLenLevels + numLitLenLevels, 0, 
            kStaticMainTableSize - numLitLenLevels);
          
          memmove(distLevels, tmpLevels + numLitLenLevels, numDistLevels);
          memset(distLevels + numDistLevels, 0, kStaticDistTableSize - numDistLevels);
        }
        try
        {
          m_MainDecoder.SetCodeLengths(litLenLevels);
          m_DistDecoder.SetCodeLengths(distLevels);
        }
        catch(...)
        {
          throw CDecoderException(CDecoderException::kData);
        }
        break;
      }
    default:
      throw CDecoderException(CDecoderException::kData);
  }
}

class CCoderReleaser
{
  CDecoder *m_Coder;
public:
  CCoderReleaser(CDecoder *aCoder): m_Coder(aCoder) {}
  ~CCoderReleaser()
  {
    m_Coder->Flush();
    m_Coder->ReleaseStreams();
  }
};

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;
  UInt64 size = *outSize;

  if (!m_OutWindowStream.Create(kHistorySize))
    return E_OUTOFMEMORY;
  if (!m_InBitStream.Create(1 << 20))
    return E_OUTOFMEMORY;

  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(false);
  
  m_InBitStream.SetStream(inStream);
  m_InBitStream.InitMain(m_ReservedSize, m_NumInDataBlocks);
  CCoderReleaser coderReleaser(this);

  UInt64 nowPos = 0;
  while(nowPos < size)
  {
    if (progress != NULL)
    {
      UInt64 packSize = m_InBitStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &nowPos));
    }
    UInt32 uncompressedCFDataBlockSize;
    bool dataAreCorrect;
    RINOK(m_InBitStream.ReadBlock(uncompressedCFDataBlockSize, dataAreCorrect));
    if (!dataAreCorrect)
    {
      throw "Data Error";
    }
    m_InBitStream.Init();
    if (m_InBitStream.ReadBits(8) != 0x43)
      throw CDecoderException(CDecoderException::kData);
    if (m_InBitStream.ReadBits(8) != 0x4B)
      throw CDecoderException(CDecoderException::kData);
    UInt32 uncompressedCFDataCurrentValue = 0;
    m_FinalBlock = false;
    while (uncompressedCFDataCurrentValue < uncompressedCFDataBlockSize)
    {
      ReadTables();
      if(m_StoredMode)
      {
        for (UInt32 i = 0; i < m_StoredBlockSize; i++)
          m_OutWindowStream.PutByte(Byte(m_InBitStream.ReadBits(8)));
        nowPos += m_StoredBlockSize;
        uncompressedCFDataCurrentValue += m_StoredBlockSize;
        continue;
      }
      while(true)
      {
        UInt32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
        
        if (number < 256)
        {
          m_OutWindowStream.PutByte(Byte(number));
          nowPos++;
          uncompressedCFDataCurrentValue++;
          continue;
        }
        else if (number >= kMatchNumber)
        {
          number -= kMatchNumber;
          UInt32 length = UInt32(kLenStart[number]) + kMatchMinLen;
          UInt32 numBits; 
          if ((numBits = kLenDirectBits[number]) > 0)
            length += m_InBitStream.ReadBits(numBits);
          
          number = m_DistDecoder.DecodeSymbol(&m_InBitStream);
          UInt32 distance = kDistStart[number] + m_InBitStream.ReadBits(kDistDirectBits[number]);
          /*
          if (distance >= nowPos)
            throw "data error";
          */
          m_OutWindowStream.CopyBlock(distance, length);
          nowPos += length;
          uncompressedCFDataCurrentValue += length;
        }
        else if (number == kReadTableNumber)
        {
          break;
        }
        else
          throw CDecoderException(CDecoderException::kData);
      }
    }
  }
  return S_OK;
}

}}}
