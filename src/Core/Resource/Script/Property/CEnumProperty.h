#ifndef CENUMPROPERTY_H
#define CENUMPROPERTY_H

#include "IProperty.h"

/** There are two types of enum properties in the game data: enum and choice.
 *
 *  In the game, the difference is that choice properties are index-based, while
 *  enum properties are stored as a hash of the name of the enum value.
 *
 *  In PWE, however, they are both implemented the same way under the hood.
 */
#define PROP_TYPE int32
#define BASE TSerializeableTypedProperty<PROP_TYPE, TypeEnum>
#define TYPED_PROPERTY_BASE TTypedProperty<PROP_TYPE, TypeEnum>
template<EPropertyType TypeEnum>
class TEnumPropertyBase : public BASE
{
    friend class IProperty;

    struct SEnumValue
    {
        TString Name;
        uint32 ID;

        SEnumValue()
            : ID(0)
        {}

        SEnumValue(const TString& rkInName, uint32 InID)
            : Name(rkInName), ID(InID) {}


        inline bool operator==(const SEnumValue& rkOther) const
        {
            return( Name == rkOther.Name && ID == rkOther.ID );
        }

        void Serialize(IArchive& rArc)
        {
            rArc << SerialParameter("Name", Name, SH_Attribute)
                 << SerialParameter("ID", ID, SH_Attribute | SH_HexDisplay);
        }
    };
    std::vector<SEnumValue> mValues;

    /** If true, the archetype's name will be used as the type name instead of "enum" or "choice". */
    bool mOverrideTypeName;

protected:
    /** Constructor */
    TEnumPropertyBase(EGame Game)
        : BASE(Game)
        , mOverrideTypeName(false)
    {}

public:
    virtual const char* HashableTypeName() const
    {
        if (IProperty::mpArchetype)
            return IProperty::mpArchetype->HashableTypeName();
        else if (mOverrideTypeName)
            return *IProperty::mName;
        else if (TypeEnum == EPropertyType::Enum)
            return "enum";
        else
            return "choice";
    }

    virtual void Serialize(IArchive& rArc)
    {
        // Skip TSerializeableTypedProperty, serialize default value ourselves so we can set SH_HexDisplay
        TYPED_PROPERTY_BASE::Serialize(rArc);

        TEnumPropertyBase* pArchetype = static_cast<TEnumPropertyBase*>(IProperty::mpArchetype);
        uint32 DefaultValueFlags = SH_HexDisplay | (pArchetype || IProperty::Game() <= EGame::Prime ? SH_Optional : 0);

        rArc << SerialParameter("DefaultValue", TYPED_PROPERTY_BASE::mDefaultValue, DefaultValueFlags, pArchetype ? pArchetype->mDefaultValue : 0);

        // Only serialize type name override for root archetypes.
        if (!IProperty::mpArchetype)
        {
            rArc << SerialParameter("OverrideTypeName", mOverrideTypeName, SH_Optional, false);
        }

        if (!pArchetype || !rArc.CanSkipParameters() || mValues != pArchetype->mValues)
        {
            rArc << SerialParameter("Values", mValues);
        }
    }

    virtual void SerializeValue(void* pData, IArchive& Arc) const
    {
        Arc.SerializePrimitive( (uint32&) TYPED_PROPERTY_BASE::ValueRef(pData), 0 );
    }

    virtual void InitFromArchetype(IProperty* pOther)
    {
        TYPED_PROPERTY_BASE::InitFromArchetype(pOther);
        TEnumPropertyBase* pOtherEnum = static_cast<TEnumPropertyBase*>(pOther);
        mValues = pOtherEnum->mValues;
    }

    virtual TString ValueAsString(void* pData) const
    {
        return TString::FromInt32( TYPED_PROPERTY_BASE::Value(pData), 0, 10 );
    }

    void AddValue(TString ValueName, uint32 ValueID)
    {
        mValues.push_back( SEnumValue(ValueName, ValueID) );
    }

    inline uint32 NumPossibleValues() const { return mValues.size(); }

    uint32 ValueIndex(uint32 ID) const
    {
        for (uint32 ValueIdx = 0; ValueIdx < mValues.size(); ValueIdx++)
        {
            if (mValues[ValueIdx].ID == ID)
            {
                return ValueIdx;
            }
        }
        return -1;
    }

    uint32 ValueID(uint32 Index) const
    {
        ASSERT(Index >= 0 && Index < mValues.size());
        return mValues[Index].ID;
    }

    TString ValueName(uint32 Index) const
    {
        ASSERT(Index >= 0 && Index < mValues.size());
        return mValues[Index].Name;
    }

    bool HasValidValue(void* pPropertyData)
    {
        if (mValues.empty()) return true;
        int ID = TYPED_PROPERTY_BASE::ValueRef(pPropertyData);
        uint32 Index = ValueIndex(ID);
        return Index >= 0 && Index < mValues.size();
    }

    bool OverridesTypeName() const
    {
        return IProperty::mpArchetype ? TPropCast<TEnumPropertyBase>(IProperty::mpArchetype)->OverridesTypeName() : mOverrideTypeName;
    }

    void SetOverrideTypeName(bool Override)
    {
        if (IProperty::mpArchetype)
        {
            TEnumPropertyBase* pArchetype = TPropCast<TEnumPropertyBase>(IProperty::RootArchetype());
            pArchetype->SetOverrideTypeName(Override);
        }
        else
        {
            if (mOverrideTypeName != Override)
            {
                mOverrideTypeName = Override;
                IProperty::MarkDirty();
            }
        }
    }
};

typedef TEnumPropertyBase<EPropertyType::Choice> CChoiceProperty;
typedef TEnumPropertyBase<EPropertyType::Enum>   CEnumProperty;

// Specialization of TPropCast to allow interchangeable casting, as both types are the same thing
template<>
inline CEnumProperty* TPropCast(IProperty* pProperty)
{
    if (pProperty)
    {
        EPropertyType InType = pProperty->Type();

        if (InType == EPropertyType::Enum || InType == EPropertyType::Choice)
        {
            return static_cast<CEnumProperty*>(pProperty);
        }
    }

    return nullptr;
}

template<>
inline CChoiceProperty* TPropCast(IProperty* pProperty)
{
    if (pProperty)
    {
        EPropertyType InType = pProperty->Type();

        if (InType == EPropertyType::Enum || InType == EPropertyType::Choice)
        {
            return static_cast<CChoiceProperty*>(pProperty);
        }
    }

    return nullptr;
}

#endif // CENUMPROPERTY_H
