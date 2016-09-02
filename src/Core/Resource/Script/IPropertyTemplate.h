#ifndef IPROPERTYTEMPLATE
#define IPROPERTYTEMPLATE

#include "EPropertyType.h"
#include "IProperty.h"
#include "IPropertyValue.h"
#include "Core/Resource/CAnimationParameters.h"
#include <Common/CColor.h>
#include <Common/TString.h>
#include <Common/types.h>
#include <Math/CVector3f.h>
#include <vector>

typedef TString TIDString;
class CMasterTemplate;
class CStructTemplate;
class IProperty;

enum ECookPreference
{
    eNoCookPreference,
    eAlwaysCook,
    eNeverCook
};

// IPropertyTemplate - Base class. Contains basic info that every property has,
// plus virtual functions for determining more specific property type.
class IPropertyTemplate
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

protected:
    CStructTemplate *mpParent;
    CScriptTemplate *mpScriptTemplate;
    CMasterTemplate *mpMasterTemplate;
    TString mName;
    TString mDescription;
    u32 mID;
    ECookPreference mCookPreference;
    std::vector<u32> mAllowedVersions;

public:
    IPropertyTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : mID(ID)
        , mpParent(pParent)
        , mpScriptTemplate(pScript)
        , mpMasterTemplate(pMaster)
        , mName("UNSET PROPERTY NAME")
        , mCookPreference(eNoCookPreference)
    {
    }

    IPropertyTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : mID(ID)
        , mpParent(pParent)
        , mpScriptTemplate(pScript)
        , mpMasterTemplate(pMaster)
        , mName(rkName)
        , mCookPreference(CookPreference)
    {
    }

    virtual EPropertyType Type()  const = 0;
    virtual bool CanHaveDefault() const = 0;
    virtual bool IsNumerical()    const = 0;
    virtual IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent) = 0;
    virtual IPropertyTemplate* Clone(CScriptTemplate *pScript, CStructTemplate *pParent = 0) const = 0;

    virtual void Copy(const IPropertyTemplate *pkTemp)
    {
        mName = pkTemp->mName;
        mDescription = pkTemp->mDescription;
        mID = pkTemp->mID;
        mCookPreference = pkTemp->mCookPreference;
        mAllowedVersions = pkTemp->mAllowedVersions;
    }

    virtual bool Matches(const IPropertyTemplate *pkTemp) const
    {
        return ( (pkTemp != nullptr) &&
                 (mName == pkTemp->mName) &&
                 (mDescription == pkTemp->mDescription) &&
                 (mID == pkTemp->mID) &&
                 (mCookPreference == pkTemp->mCookPreference) &&
                 (mAllowedVersions == pkTemp->mAllowedVersions) &&
                 (Type() == pkTemp->Type()) );
    }

    virtual TString DefaultToString() const                 { return ""; }
    virtual const IPropertyValue* RawDefaultValue() const   { return nullptr; }
    virtual bool HasValidRange() const                      { return false; }
    virtual TString RangeToString()   const                 { return ""; }
    virtual TString Suffix() const                          { return ""; }

    virtual void SetParam(const TString& rkParamName, const TString& rkValue)
    {
        if (rkParamName == "cook_pref")
        {
            TString lValue = rkValue.ToLower();

            if (lValue == "always")
                mCookPreference = eAlwaysCook;
            else if (lValue == "never")
                mCookPreference = eNeverCook;
            else
                mCookPreference = eNoCookPreference;
        }

        else if (rkParamName == "description")
            mDescription = rkValue;
    }

    EGame Game() const;
    bool IsInVersion(u32 Version) const;
    TIDString IDString(bool FullPath) const;
    bool IsDescendantOf(const CStructTemplate *pStruct) const;
    bool IsFromStructTemplate() const;
    TString FindStructSource() const;
    CStructTemplate* RootStruct();

    // Inline Accessors
    inline TString Name() const                         { return mName; }
    inline TString Description() const                  { return mDescription; }
    inline u32 PropertyID() const                       { return mID; }
    inline ECookPreference CookPreference() const       { return mCookPreference; }
    inline CStructTemplate* Parent() const              { return mpParent; }
    inline CScriptTemplate* ScriptTemplate() const      { return mpScriptTemplate; }
    inline CMasterTemplate* MasterTemplate() const      { return mpMasterTemplate; }
    inline void SetName(const TString& rkName)          { mName = rkName; }
    inline void SetDescription(const TString& rkDesc)   { mDescription = rkDesc; }
};

// Macro for defining reimplementations of IPropertyTemplate::Clone(), which are usually identical to each other aside from the class being instantiated
#define IMPLEMENT_TEMPLATE_CLONE(ClassName) \
    virtual IPropertyTemplate* Clone(CScriptTemplate *pScript, CStructTemplate *pParent = 0) const \
    { \
        if (!pParent) pParent = mpParent; \
        if (!pScript) pScript = mpScriptTemplate; \
        ClassName *pTemp = new ClassName(mID, pScript, mpMasterTemplate, pParent); \
        pTemp->Copy(this); \
        return pTemp; \
    }

// TTypedPropertyTemplate - Template property class that allows for tracking
// a default value. Typedefs are set up for a bunch of property types.
template<typename PropType, EPropertyType PropTypeEnum, class ValueClass, bool CanHaveDefaultValue>
class TTypedPropertyTemplate : public IPropertyTemplate
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

protected:
    ValueClass mDefaultValue;

public:
    TTypedPropertyTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : IPropertyTemplate(ID, pScript, pMaster, pParent) {}

    TTypedPropertyTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : IPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent) {}

    virtual EPropertyType Type()  const { return PropTypeEnum;          }
    virtual bool CanHaveDefault() const { return CanHaveDefaultValue;   }
    virtual bool IsNumerical()    const { return false;                 }

    virtual IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        typedef TTypedProperty<PropType, PropTypeEnum, ValueClass> TPropertyType;
        TPropertyType *pOut = new TPropertyType(this, pInstance, pParent, GetDefaultValue());
        return pOut;
    }

    IMPLEMENT_TEMPLATE_CLONE(TTypedPropertyTemplate)

    virtual void Copy(const IPropertyTemplate *pkTemp)
    {
        IPropertyTemplate::Copy(pkTemp);
        mDefaultValue.Copy(&static_cast<const TTypedPropertyTemplate*>(pkTemp)->mDefaultValue);
    }

    virtual bool Matches(const IPropertyTemplate *pkTemp) const
    {
        const TTypedPropertyTemplate *pkTyped = static_cast<const TTypedPropertyTemplate*>(pkTemp);

        return ( (IPropertyTemplate::Matches(pkTemp)) &&
                 (mDefaultValue.Matches(&pkTyped->mDefaultValue)) );
    }

    virtual TString DefaultToString() const
    {
        return mDefaultValue.ToString();
    }

    virtual const IPropertyValue* RawDefaultValue() const
    {
        return &mDefaultValue;
    }

    virtual void SetParam(const TString& rkParamName, const TString& rkValue)
    {
        IPropertyTemplate::SetParam(rkParamName, rkValue);

        if (rkParamName == "default")
            mDefaultValue.FromString(rkValue.ToLower());
    }

    inline PropType GetDefaultValue() const             { return mDefaultValue.Get(); }
    inline void SetDefaultValue(const PropType& rkIn)   { mDefaultValue.Set(rkIn); }
};

// TNumericalPropertyTemplate - Subclass of TTypedPropertyTemplate for numerical
// property types, and allows a min/max value and a suffix to be tracked.
template<typename PropType, EPropertyType PropTypeEnum, class ValueClass>
class TNumericalPropertyTemplate : public TTypedPropertyTemplate<PropType,PropTypeEnum,ValueClass,true>
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

    ValueClass mMin;
    ValueClass mMax;
    TString mSuffix;

public:
    TNumericalPropertyTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, pScript, pMaster, pParent)
    {}

    TNumericalPropertyTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent)
        , mMin(0)
        , mMax(0)
    {}

    virtual bool IsNumerical() const    { return true; }
    virtual bool HasValidRange() const  { return (mMin != 0 || mMax != 0); }

    IMPLEMENT_TEMPLATE_CLONE(TNumericalPropertyTemplate)

    virtual void Copy(const IPropertyTemplate *pkTemp)
    {
        TTypedPropertyTemplate::Copy(pkTemp);

        const TNumericalPropertyTemplate *pkNumerical = static_cast<const TNumericalPropertyTemplate*>(pkTemp);
        mMin.Copy(&pkNumerical->mMin);
        mMax.Copy(&pkNumerical->mMax);
        mSuffix = pkNumerical->mSuffix;
    }

    virtual bool Matches(const IPropertyTemplate *pkTemp) const
    {
        const TNumericalPropertyTemplate *pkNumerical = static_cast<const TNumericalPropertyTemplate*>(pkTemp);

        return ( (TTypedPropertyTemplate::Matches(pkTemp)) &&
                 (mMin.Matches(&pkNumerical->mMin)) &&
                 (mMax.Matches(&pkNumerical->mMax)) &&
                 (mSuffix == pkNumerical->mSuffix) );
    }

    virtual TString RangeToString() const
    {
        return mMin.ToString() + "," + mMax.ToString();
    }

    virtual void SetParam(const TString& rkParamName, const TString& rkValue)
    {
        TTypedPropertyTemplate::SetParam(rkParamName, rkValue);

        if (rkParamName == "range")
        {
            TStringList Components = rkValue.ToLower().Split(", ");

            if (Components.size() == 2)
            {
                mMin.FromString(Components.front());
                mMax.FromString(Components.back());
            }
        }

        else if (rkParamName == "suffix")
        {
            mSuffix = rkValue;
        }
    }

    virtual TString Suffix() const { return mSuffix; }
    inline PropType GetMin() const { return mMin.Get(); }
    inline PropType GetMax() const { return mMax.Get(); }

    inline void SetRange(const PropType& rkMin, const PropType& rkMax)
    {
        mMin.Set(rkMin);
        mMax.Set(rkMax);
    }

    inline void SetSuffix(const TString& rkSuffix)
    {
        mSuffix = rkSuffix;
    }
};

// Typedefs for all property types that don't need further functionality.
typedef TTypedPropertyTemplate<bool, eBoolProperty, CBoolValue, true>                               TBoolTemplate;
typedef TNumericalPropertyTemplate<s8, eByteProperty, CByteValue>                                   TByteTemplate;
typedef TNumericalPropertyTemplate<s16, eShortProperty, CShortValue>                                TShortTemplate;
typedef TNumericalPropertyTemplate<s32, eLongProperty, CLongValue>                                  TLongTemplate;
typedef TNumericalPropertyTemplate<float, eFloatProperty, CFloatValue>                              TFloatTemplate;
typedef TTypedPropertyTemplate<CVector3f, eVector3Property, CVector3Value, true>                    TVector3Template;
typedef TTypedPropertyTemplate<CColor, eColorProperty, CColorValue, true>                           TColorTemplate;

// TCharacterTemplate, TSoundTemplate, TStringTemplate, and TMayaSplineTemplate get their own subclasses so they can reimplement a couple functions
class TCharacterTemplate : public TTypedPropertyTemplate<CAnimationParameters, eCharacterProperty, CCharacterValue, false>
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

public:
    TCharacterTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, pScript, pMaster, pParent) {}

    TCharacterTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent) {}

    IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        return new TCharacterProperty(this, pInstance, pParent, CAnimationParameters(Game()));
    }
};

class TSoundTemplate : public TTypedPropertyTemplate<u32, eSoundProperty, CSoundValue, false>
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

public:
    TSoundTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, pScript, pMaster, pParent) {}

    TSoundTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent) {}

    IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        return new TSoundProperty(this, pInstance, pParent, -1);
    }
};

class TStringTemplate : public TTypedPropertyTemplate<TString, eStringProperty, CStringValue, false>
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

public:
    TStringTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, pScript, pMaster, pParent) {}

    TStringTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent) {}

    IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        return new TStringProperty(this, pInstance, pParent);
    }
};

class TMayaSplineTemplate : public TTypedPropertyTemplate<std::vector<u8>, eMayaSplineProperty, CMayaSplineValue, false>
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

public:
    TMayaSplineTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, pScript, pMaster, pParent) {}

    TMayaSplineTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent) {}

    IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        return new TMayaSplineProperty(this, pInstance, pParent);
    }
};

// CAssetTemplate - Property template for assets. Tracks a list of resource types that
// the property is allowed to accept.
class CAssetTemplate : public IPropertyTemplate
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

    TStringList mAcceptedExtensions;
public:
    CAssetTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : IPropertyTemplate(ID, pScript, pMaster, pParent) {}

    CAssetTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : IPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent) {}

    virtual EPropertyType Type() const  { return eAssetProperty; }
    virtual bool CanHaveDefault() const { return false; }
    virtual bool IsNumerical() const    { return false; }

    IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        return new TAssetProperty(this, pInstance, pParent);
    }

    IMPLEMENT_TEMPLATE_CLONE(CAssetTemplate)

    virtual void Copy(const IPropertyTemplate *pkTemp)
    {
        IPropertyTemplate::Copy(pkTemp);
        mAcceptedExtensions = static_cast<const CAssetTemplate*>(pkTemp)->mAcceptedExtensions;
    }

    virtual bool Matches(const IPropertyTemplate *pkTemp) const
    {
        const CAssetTemplate *pkAsset = static_cast<const CAssetTemplate*>(pkTemp);

        return ( (IPropertyTemplate::Matches(pkTemp)) &&
                 (mAcceptedExtensions == pkAsset->mAcceptedExtensions) );
    }

    bool AcceptsExtension(const TString& rkExtension)
    {
        for (auto it = mAcceptedExtensions.begin(); it != mAcceptedExtensions.end(); it++)
            if (*it == rkExtension) return true;
        return false;
    }

    void SetAllowedExtensions(const TStringList& rkExtensions)  { mAcceptedExtensions = rkExtensions; }
    const TStringList& AllowedExtensions() const                { return mAcceptedExtensions; }
};

// CEnumTemplate - Property template for enums. Tracks a list of possible values (enumerators).
class CEnumTemplate : public TTypedPropertyTemplate<s32, eEnumProperty, CHexLongValue, true>
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

    struct SEnumerator
    {
        TString Name;
        u32 ID;

        SEnumerator(const TString& rkName, u32 _ID)
            : Name(rkName), ID(_ID) {}

        bool operator==(const SEnumerator& rkOther) const
        {
            return ( (Name == rkOther.Name) && (ID == rkOther.ID) );
        }
    };
    std::vector<SEnumerator> mEnumerators;
    TString mSourceFile;

public:
    CEnumTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, pScript, pMaster, pParent)
    {
    }

    CEnumTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent)
    {
    }

    virtual EPropertyType Type()  const { return eEnumProperty; }
    virtual bool CanHaveDefault() const { return true; }
    virtual bool IsNumerical()    const { return false; }

    virtual IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        TEnumProperty *pEnum = new TEnumProperty(this, pInstance, pParent);
        pEnum->Set(GetDefaultValue());
        return pEnum;
    }

    IMPLEMENT_TEMPLATE_CLONE(CEnumTemplate)

    virtual void Copy(const IPropertyTemplate *pkTemp)
    {
        TTypedPropertyTemplate::Copy(pkTemp);

        const CEnumTemplate *pkEnum = static_cast<const CEnumTemplate*>(pkTemp);
        mEnumerators = pkEnum->mEnumerators;
        mSourceFile = pkEnum->mSourceFile;
    }

    virtual bool Matches(const IPropertyTemplate *pkTemp) const
    {
        const CEnumTemplate *pkEnum = static_cast<const CEnumTemplate*>(pkTemp);

        return ( (TTypedPropertyTemplate::Matches(pkTemp)) &&
                 (mEnumerators == pkEnum->mEnumerators) &&
                 (mSourceFile == pkEnum->mSourceFile) );
    }

    inline TString SourceFile() const { return mSourceFile; }
    inline u32 NumEnumerators() const { return mEnumerators.size(); }

    u32 EnumeratorIndex(u32 enumID) const
    {
        for (u32 iEnum = 0; iEnum < mEnumerators.size(); iEnum++)
        {
            if (mEnumerators[iEnum].ID == enumID)
                return iEnum;
        }
        return -1;
    }

    u32 EnumeratorID(u32 enumIndex) const
    {
        if (mEnumerators.size() > enumIndex)
            return mEnumerators[enumIndex].ID;

        else return -1;
    }

    TString EnumeratorName(u32 enumIndex) const
    {
        if (mEnumerators.size() > enumIndex)
            return mEnumerators[enumIndex].Name;

        else return "INVALID ENUM INDEX";
    }
};

// CBitfieldTemplate - Property template for bitfields, which can have multiple
// distinct boolean parameters packed into one property.
class CBitfieldTemplate : public TTypedPropertyTemplate<u32, eBitfieldProperty, CHexLongValue, true>
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

    struct SBitFlag
    {
        TString Name;
        u32 Mask;

        SBitFlag(const TString& _name, u32 _mask)
            : Name(_name), Mask(_mask) {}

        bool operator==(const SBitFlag& rkOther) const
        {
            return ( (Name == rkOther.Name) && (Mask == rkOther.Mask) );
        }
    };
    std::vector<SBitFlag> mBitFlags;
    TString mSourceFile;

public:
    CBitfieldTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, pScript, pMaster, pParent)
    {
    }

    CBitfieldTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : TTypedPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent)
    {
    }

    virtual EPropertyType Type()  const { return eBitfieldProperty; }
    virtual bool CanHaveDefault() const { return true; }
    virtual bool IsNumerical()    const { return false; }

    virtual IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        TBitfieldProperty *pBitfield = new TBitfieldProperty(this, pInstance, pParent);
        pBitfield->Set(GetDefaultValue());
        return pBitfield;
    }

    IMPLEMENT_TEMPLATE_CLONE(CBitfieldTemplate)

    virtual void Copy(const IPropertyTemplate *pkTemp)
    {
        TTypedPropertyTemplate::Copy(pkTemp);

        const CBitfieldTemplate *pkBitfield = static_cast<const CBitfieldTemplate*>(pkTemp);
        mBitFlags = pkBitfield->mBitFlags;
        mSourceFile = pkBitfield->mSourceFile;
    }

    virtual bool Matches(const IPropertyTemplate *pkTemp) const
    {
        const CBitfieldTemplate *pkBitfield = static_cast<const CBitfieldTemplate*>(pkTemp);

        return ( (TTypedPropertyTemplate::Matches(pkTemp)) &&
                 (mBitFlags == pkBitfield->mBitFlags) &&
                 (mSourceFile == pkBitfield->mSourceFile) );
    }

    inline TString SourceFile() const           { return mSourceFile; }
    inline u32 NumFlags() const                 { return mBitFlags.size(); }
    inline TString FlagName(u32 index) const    { return mBitFlags[index].Name; }
    inline u32 FlagMask(u32 index) const        { return mBitFlags[index].Mask; }
};

// CStructTemplate - Defines structs composed of multiple sub-properties.
class CStructTemplate : public IPropertyTemplate
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;

protected:
    std::vector<IPropertyTemplate*> mSubProperties;
    std::vector<u32> mVersionPropertyCounts;
    bool mIsSingleProperty;
    TString mSourceFile;

    void DetermineVersionPropertyCounts();
public:
    CStructTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : IPropertyTemplate(ID, pScript, pMaster, pParent)
        , mIsSingleProperty(false) {}

    CStructTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : IPropertyTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent)
        , mIsSingleProperty(false) {}

    ~CStructTemplate()
    {
        for (auto it = mSubProperties.begin(); it != mSubProperties.end(); it++)
            delete *it;
    }

    EPropertyType Type()  const { return eStructProperty; }
    bool CanHaveDefault() const { return false; }
    bool IsNumerical()    const { return false; }

    IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        CPropertyStruct *pStruct = new CPropertyStruct(this, pInstance, pParent);

        for (u32 iSub = 0; iSub < mSubProperties.size(); iSub++)
        {
            IProperty *pSubProp = mSubProperties[iSub]->InstantiateProperty(pInstance, pStruct);
            pStruct->AddSubProperty(pSubProp);
        }

        return pStruct;
    }

    IMPLEMENT_TEMPLATE_CLONE(CStructTemplate)

    virtual void Copy(const IPropertyTemplate *pkTemp)
    {
        IPropertyTemplate::Copy(pkTemp);

        const CStructTemplate *pkStruct = static_cast<const CStructTemplate*>(pkTemp);
        CopyStructData(pkStruct);
    }

    void CopyStructData(const CStructTemplate *pkStruct);

    virtual bool Matches(const IPropertyTemplate *pkTemp) const
    {
        const CStructTemplate *pkStruct = static_cast<const CStructTemplate*>(pkTemp);

        if ( (IPropertyTemplate::Matches(pkTemp)) &&
             (mVersionPropertyCounts == pkStruct->mVersionPropertyCounts) &&
             (mIsSingleProperty == pkStruct->mIsSingleProperty) &&
             (mSourceFile == pkStruct->mSourceFile) )
        {
            return StructDataMatches(pkStruct);
        }

        return false;
    }

    bool StructDataMatches(const CStructTemplate *pkStruct) const
    {
        if ( (mIsSingleProperty == pkStruct->mIsSingleProperty) &&
             (mSubProperties.size() == pkStruct->mSubProperties.size()) )
        {
            for (u32 iSub = 0; iSub < mSubProperties.size(); iSub++)
            {
                if (!mSubProperties[iSub]->Matches(pkStruct->mSubProperties[iSub]))
                    return false;
            }

            return true;
        }

        return false;
    }

    inline TString SourceFile() const               { return mSourceFile; }
    inline bool IsSingleProperty() const            { return mIsSingleProperty; }
    inline u32 Count() const                        { return mSubProperties.size(); }
    inline u32 NumVersions() const                  { return mVersionPropertyCounts.size(); }

    u32 PropertyCountForVersion(u32 Version);
    u32 VersionForPropertyCount(u32 PropCount);
    IPropertyTemplate* PropertyByIndex(u32 index);
    IPropertyTemplate* PropertyByID(u32 ID);
    IPropertyTemplate* PropertyByIDString(const TIDString& str);
    CStructTemplate* StructByIndex(u32 index);
    CStructTemplate* StructByID(u32 ID);
    CStructTemplate* StructByIDString(const TIDString& str);
    bool HasProperty(const TIDString& rkIdString);
    void DebugPrintProperties(TString base);
};

// CArrayTemplate - Defines a repeating struct composed of multiple sub-properties.
// Similar to CStructTemplate, but with new implementations of Type() and InstantiateProperty().
class CArrayTemplate : public CStructTemplate
{
    friend class CTemplateLoader;
    friend class CTemplateWriter;
    TString mElementName;

public:
    CArrayTemplate(u32 ID, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : CStructTemplate(ID, pScript, pMaster, pParent)
    {
        mIsSingleProperty = true;
    }

    CArrayTemplate(u32 ID, const TString& rkName, ECookPreference CookPreference, CScriptTemplate *pScript, CMasterTemplate *pMaster, CStructTemplate *pParent = 0)
        : CStructTemplate(ID, rkName, CookPreference, pScript, pMaster, pParent)
    {
        mIsSingleProperty = true;
    }

    EPropertyType Type() const { return eArrayProperty; }

    IProperty* InstantiateProperty(CScriptObject *pInstance, CPropertyStruct *pParent)
    {
        return new CArrayProperty(this, pInstance, pParent);
    }

    IMPLEMENT_TEMPLATE_CLONE(CArrayTemplate)

    virtual void Copy(const IPropertyTemplate *pkTemp)
    {
        CStructTemplate::Copy(pkTemp);
        mElementName = static_cast<const CArrayTemplate*>(pkTemp)->mElementName;
    }

    virtual bool Matches(const IPropertyTemplate *pkTemp) const
    {
        const CArrayTemplate *pkArray = static_cast<const CArrayTemplate*>(pkTemp);

        return ( (mElementName == pkArray->mElementName) &
                 (CStructTemplate::Matches(pkTemp)) );
    }

    void SetParam(const TString& rkParamName, const TString& rkValue)
    {
        if (rkParamName == "element_name")
            mElementName = rkValue;
        else
            CStructTemplate::SetParam(rkParamName, rkValue);
    }

    TString ElementName() const                { return mElementName; }
    void SetElementName(const TString& rkName) { mElementName = rkName; }

    CPropertyStruct* CreateSubStruct(CScriptObject *pInstance, CArrayProperty *pArray)
    {
        return (CPropertyStruct*) CStructTemplate::InstantiateProperty(pInstance, pArray);
    }
};

#endif // IPROPERTYTEMPLATE

