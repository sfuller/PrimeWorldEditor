#include "CStringLoader.h"
#include <Common/Log.h>

void CStringLoader::LoadPrimeDemoSTRG(IInputStream& rSTRG)
{
    // This function starts at 0x4 in the file - right after the size
    // This STRG version only supports one language per file
    mpStringTable->mLangTables.resize(1);
    CStringTable::SLangTable* Lang = &mpStringTable->mLangTables[1];
    Lang->Language = "ENGL";
    uint32 TableStart = rSTRG.Tell();

    // Header
    uint32 NumStrings = rSTRG.ReadLong();
    Lang->Strings.resize(NumStrings);
    mpStringTable->mNumStrings = NumStrings;

    // String offsets (yeah, that wasn't much of a header)
    std::vector<uint32> StringOffsets(NumStrings);
    for (uint32 iOff = 0; iOff < StringOffsets.size(); iOff++)
        StringOffsets[iOff] = rSTRG.ReadLong();

    // Strings
    for (uint32 iStr = 0; iStr < NumStrings; iStr++)
    {
        rSTRG.Seek(TableStart + StringOffsets[iStr], SEEK_SET);
        Lang->Strings[iStr] = rSTRG.ReadString();
    }
}

void CStringLoader::LoadPrimeSTRG(IInputStream& rSTRG)
{
    // This function starts at 0x8 in the file, after magic/version
    // Header
    uint32 NumLanguages = rSTRG.ReadLong();
    uint32 NumStrings = rSTRG.ReadLong();
    mpStringTable->mNumStrings = NumStrings;

    // Language definitions
    mpStringTable->mLangTables.resize(NumLanguages);
    std::vector<uint32> LangOffsets(NumLanguages);

    for (uint32 iLang = 0; iLang < NumLanguages; iLang++)
    {
        mpStringTable->mLangTables[iLang].Language = CFourCC(rSTRG);
        LangOffsets[iLang] = rSTRG.ReadLong();
        if (mVersion == EGame::Echoes) rSTRG.Seek(0x4, SEEK_CUR); // Skipping strings size
    }

    // String names
    if (mVersion == EGame::Echoes)
        LoadNameTable(rSTRG);

    // Strings
    uint32 StringsStart = rSTRG.Tell();
    for (uint32 iLang = 0; iLang < NumLanguages; iLang++)
    {
        rSTRG.Seek(StringsStart + LangOffsets[iLang], SEEK_SET);
        if (mVersion == EGame::Prime) rSTRG.Seek(0x4, SEEK_CUR); // Skipping strings size

        uint32 LangStart = rSTRG.Tell();
        CStringTable::SLangTable* pLang = &mpStringTable->mLangTables[iLang];
        pLang->Strings.resize(NumStrings);

        // Offsets
        std::vector<uint32> StringOffsets(NumStrings);
        for (uint32 iOff = 0; iOff < NumStrings; iOff++)
            StringOffsets[iOff] = rSTRG.ReadLong();

        // The actual strings
        for (uint32 iStr = 0; iStr < NumStrings; iStr++)
        {
            rSTRG.Seek(LangStart + StringOffsets[iStr], SEEK_SET);
            pLang->Strings[iStr] = rSTRG.ReadString();
        }
    }
}

void CStringLoader::LoadCorruptionSTRG(IInputStream& rSTRG)
{
    // This function starts at 0x8 in the file, after magic/version
    // Header
    uint32 NumLanguages = rSTRG.ReadLong();
    uint32 NumStrings = rSTRG.ReadLong();
    mpStringTable->mNumStrings = NumStrings;

    // String names
    LoadNameTable(rSTRG);

    // Language definitions
    mpStringTable->mLangTables.resize(NumLanguages);
    std::vector<std::vector<uint32>> LangOffsets(NumLanguages);

    for (uint32 iLang = 0; iLang < NumLanguages; iLang++)
        mpStringTable->mLangTables[iLang].Language = CFourCC(rSTRG);

    for (uint32 iLang = 0; iLang < NumLanguages; iLang++)
    {
        LangOffsets[iLang].resize(NumStrings);

        rSTRG.Seek(0x4, SEEK_CUR); // Skipping total string size

        for (uint32 iStr = 0; iStr < NumStrings; iStr++)
            LangOffsets[iLang][iStr] = rSTRG.ReadLong();
    }

    // Strings
    uint32 StringsStart = rSTRG.Tell();

    for (uint32 iLang = 0; iLang < NumLanguages; iLang++)
    {
        CStringTable::SLangTable *pLang = &mpStringTable->mLangTables[iLang];
        pLang->Strings.resize(NumStrings);

        for (uint32 iStr = 0; iStr < NumStrings; iStr++)
        {
            rSTRG.Seek(StringsStart + LangOffsets[iLang][iStr], SEEK_SET);
            rSTRG.Seek(0x4, SEEK_CUR); // Skipping string size

            pLang->Strings[iStr] = rSTRG.ReadString();
        }
    }
}

void CStringLoader::LoadNameTable(IInputStream& rSTRG)
{
    // Name table header
    uint32 NameCount = rSTRG.ReadLong();
    uint32 NameTableSize = rSTRG.ReadLong();
    uint32 NameTableStart = rSTRG.Tell();
    uint32 NameTableEnd = NameTableStart + NameTableSize;

    // Name definitions
    struct SNameDef {
        uint32 NameOffset, StringIndex;
    };
    std::vector<SNameDef> NameDefs(NameCount);

    for (uint32 iName = 0; iName < NameCount; iName++)
    {
        NameDefs[iName].NameOffset = rSTRG.ReadLong() + NameTableStart;
        NameDefs[iName].StringIndex = rSTRG.ReadLong();
    }

    // Name strings
    mpStringTable->mStringNames.resize(mpStringTable->mNumStrings);
    for (uint32 iName = 0; iName < NameCount; iName++)
    {
        SNameDef *pDef = &NameDefs[iName];
        rSTRG.Seek(pDef->NameOffset, SEEK_SET);
        mpStringTable->mStringNames[pDef->StringIndex] = rSTRG.ReadString();
    }
    rSTRG.Seek(NameTableEnd, SEEK_SET);
}

// ************ STATIC ************
CStringTable* CStringLoader::LoadSTRG(IInputStream& rSTRG, CResourceEntry *pEntry)
{
    // Verify that this is a valid STRG
    if (!rSTRG.IsValid()) return nullptr;

    uint32 Magic = rSTRG.ReadLong();
    EGame Version = EGame::Invalid;

    if (Magic != 0x87654321)
    {
        // Check for MP1 Demo STRG format - no magic/version; the first value is actually the filesize
        // so the best I can do is verify the first value actually points to the end of the file
        if (Magic <= (uint32) rSTRG.Size())
        {
            rSTRG.Seek(Magic, SEEK_SET);
            if ((rSTRG.EoF()) || (rSTRG.ReadShort() == 0xFFFF))
                Version = EGame::PrimeDemo;
        }

        if (Version != EGame::PrimeDemo)
        {
            errorf("%s: Invalid STRG magic: 0x%08X", *rSTRG.GetSourceString(), Magic);
            return nullptr;
        }
    }

    else
    {
        uint32 FileVersion = rSTRG.ReadLong();
        Version = GetFormatVersion(FileVersion);

        if (Version == EGame::Invalid)
        {
            errorf("%s: Unsupported STRG version: 0x%X", *rSTRG.GetSourceString(), FileVersion);
            return nullptr;
        }
    }

    // Valid; now we create the loader and call the function that reads the rest of the file
    CStringLoader Loader;
    Loader.mpStringTable = new CStringTable(pEntry);
    Loader.mVersion = Version;

    if (Version == EGame::PrimeDemo) Loader.LoadPrimeDemoSTRG(rSTRG);
    else if (Version < EGame::Corruption) Loader.LoadPrimeSTRG(rSTRG);
    else Loader.LoadCorruptionSTRG(rSTRG);

    return Loader.mpStringTable;
}

EGame CStringLoader::GetFormatVersion(uint32 Version)
{
    switch (Version)
    {
    case 0x0: return EGame::Prime;
    case 0x1: return EGame::Echoes;
    case 0x3: return EGame::Corruption;
    default: return EGame::Invalid;
    }
}
