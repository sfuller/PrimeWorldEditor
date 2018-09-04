#ifndef IARCHIVE
#define IARCHIVE

#include "CSerialVersion.h"
#include "Common/AssertMacro.h"
#include "Common/CAssetID.h"
#include "Common/CFourCC.h"
#include "Common/EGame.h"
#include "Common/TString.h"
#include "Common/types.h"

#include <type_traits>

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

/* This is a custom serialization implementation intended for saving game assets out to editor-
 * friendly formats, such as XML. The main goals of the serialization system is to simplify the
 * code for reading and writing editor files and to be able to easily update those files without
 * breaking compatibility with older versions. Support for new output formats can be added by
 * implementing new subclasses of IArchive.
 *
 * To use a class with the serialization system, it must have a Serialize function implemented.
 * There are two ways this function can be defined:
 * 1. As a member function with this signature: void Serialize(IArchive&)
 * 2. As a global/friend function with this signature: void Serialize(IArchive&, YourClass&)
 *
 * Use the << operator to serialize data members to the archive. All data being serialized must be
 * wrapped in one of the SERIAL macros. The SERIAL macro ensures that all serialized parameters are
 * associated with a name. This name makes the output file more easily human-readable as well as helps
 * ensure that files are easily backwards-compatible if parameters are moved or added/removed.
 *
 * Polymorphism is supported. There are two requirements for a polymorphic class to work with the
 * serialization system. First, the base class must contain a virtual Type() function that returns
 * an integral value (an enum or an integer), as well as a virtual Serialize(IArchive&) function.
 * Second, there must be a factory object with a SpawnObject(u32) method that takes the same Type value
 * and returns an object of the correct class.
 *
 * Containers are also supported. Containers require a different macro that allows you to specify the
 * name of the elements in the container. The currently-supported containers are std::vector, std::list,
 * and std::set. Support for more container types can be added to the bottom of this file.
 *
 * These are the available SERIAL macros:
 * - SERIAL(ParamName, ParamValue)                                          [generic parameter]
 * - SERIAL_HEX(ParamName, ParamValue)                                      [integral parameter serialized as a hex number for improved readability]
 * - SERIAL_ABSTRACT(ParamName, ParamValue, Factory)                        [polymorphic parameter]
 * - SERIAL_CONTAINER(ParamName, Container, ElementName)                    [container parameter]
 * - SERIAL_ABSTRACT_CONTAINER(ParamName, Container, ElementName, Factory)  [container of polymorphic objects parameter]
 *
 * Each of these has a variant with _AUTO at the end that allows you to exclude ParamName (the name of the
 * variable will be used as the parameter name instead).
 */

/** ESerialHint - Parameter hint flags */
enum ESerialHint
{
    SH_HexDisplay           = 0x1,      // The parameter should be written in hex in text formats
    SH_Optional             = 0x2,      // The parameter should not be written to the file if its value matches the default value
    SH_NeverSave            = 0x4,      // The parameter should not be saved to files
    SH_AlwaysSave           = 0x8,      // The parameter should always be saved regardless of if it matches the default value
    SH_Attribute            = 0x10,     // The parameter is an attribute of another parameter. Attributes cannot have children.
    SH_IgnoreName           = 0x20,     // The parameter name will not be used to validate file data. May yield incorrect results if used improperly!
};

/** EArchiveFlags */
enum EArchiveFlags
{
    AF_Reader               = 0x1,      // Archive reads data
    AF_Writer               = 0x2,      // Archive writes data
    AF_Text                 = 0x4,      // Archive reads/writes to a text format
    AF_Binary               = 0x8,      // Archive reads/writes to a binary format
    AF_NoSkipping           = 0x10,     // Properties are never skipped
};

/** Shortcut macro for enable_if */
#define ENABLE_IF(Conditions, ReturnType) typename std::enable_if< Conditions, ReturnType >::type

/** Check for whether the equality operator has been implemented for a given type */
template<typename ValType, class = decltype(std::declval<ValType>() == std::declval<ValType>())>
std::true_type  THasEqualToOperator(const ValType&);
std::false_type THasEqualToOperator(...);
template<typename ValType> using THasEqualTo = decltype(THasEqualToOperator(std::declval<ValType>()));

/** Class that determines if the type is a container */
template<typename>      struct TIsContainer : std::false_type {};
template<typename T>    struct TIsContainer< std::vector<T> > : std::true_type {};
template<typename T>    struct TIsContainer< std::list<T> > : std::true_type {};
template<typename T>    struct TIsContainer< std::set<T> > : std::true_type {};
template<typename T, typename V>    struct TIsContainer< std::map<T,V> > : std::true_type {};
template<typename T, typename V>    struct TIsContainer< std::unordered_map<T,V> > : std::true_type {};

/** Helper macro that tells us whether the parameter supports default property values */
#define SUPPORTS_DEFAULT_VALUES (!std::is_pointer_v<ValType> && std::is_copy_assignable_v<ValType> && THasEqualTo<ValType>::value && !TIsContainer<ValType>::value)

/** TSerialParameter - name/value pair for generic serial parameters */
template<typename ValType>
struct TSerialParameter
{
    const char*         pkName;
    ValType&            rValue;
    u32                 HintFlags;
    const ValType*      pDefaultValue;
};

/** Function that creates a SerialParameter */
template<typename ValType>
ENABLE_IF( SUPPORTS_DEFAULT_VALUES, TSerialParameter<ValType> )
inline SerialParameter(const char* pkName, ValType& rValue, u32 HintFlags = 0, const ValType& rkDefaultValue = ValType())
{
    return TSerialParameter<ValType> { pkName, rValue, HintFlags, &rkDefaultValue };
}
template<typename ValType>
ENABLE_IF( !SUPPORTS_DEFAULT_VALUES, TSerialParameter<ValType> )
inline SerialParameter(const char* pkName, ValType& rValue, u32 HintFlags = 0)
{
    return TSerialParameter<ValType> { pkName, rValue, HintFlags, nullptr };
}

/** Returns whether the parameter value matches its default value */
template<typename ValType>
ENABLE_IF( SUPPORTS_DEFAULT_VALUES, bool )
inline ParameterMatchesDefault( const TSerialParameter<ValType>& kParameter )
{
    return kParameter.pDefaultValue != nullptr && kParameter.rValue == *kParameter.pDefaultValue;
}

template<typename ValType>
ENABLE_IF( !SUPPORTS_DEFAULT_VALUES, bool )
inline ParameterMatchesDefault( const TSerialParameter<ValType>& )
{
    return false;
}

/** Initializes the parameter to its default value */
template<typename ValType>
ENABLE_IF( SUPPORTS_DEFAULT_VALUES, bool )
inline InitParameterToDefault( TSerialParameter<ValType>& Param )
{
    if (Param.pDefaultValue != nullptr)
    {
        Param.rValue = *Param.pDefaultValue;
        return true;
    }
    else
        return false;
}

template<typename ValType>
ENABLE_IF( !SUPPORTS_DEFAULT_VALUES, bool )
inline InitParameterToDefault( TSerialParameter<ValType>& )
{
    return false;
}

/** SFINAE serialize function type check */
// https://jguegant.github.io/blogs/tech/sfinae-introduction.html
void Serialize(); // This needs to be here or else the global Serialize method handling causes a compiler error. Not sure of a better fix
void BulkSerialize();

/** Helper struct to verify that function Func exists and matches the given function signature (FuncType) */
template<typename FuncType, FuncType Func> struct FunctionExists;

/** Determine what kind of Serialize function the type has */
template<typename ValueType, class ArchiveType>
struct SerialType
{
    enum { Primitive, Member, Global, None };

    // Check for ArcType::SerializePrimitive(ValType&) method
    template<typename ValType, typename ArcType> static long long&  Check(FunctionExists<void (ArcType::*)(ValType&, u32), &ArcType::SerializePrimitive>*);
    // Check for ValType::Serialize(ArcType&)
    template<typename ValType, typename ArcType> static long&       Check(FunctionExists<void (ValType::*)(ArcType&), &ValType::Serialize>*);
    // Check for global Serialize(ArcType&,ValType&) function
    template<typename ValType, typename ArcType> static short&      Check(FunctionExists<void (*)(ArcType&, ValType&), &Serialize>*);
    // Fallback - no valid Serialize method exists
    template<typename ValType, typename ArcType> static char&       Check(...);

    // Set Type enum to correspond to whichever function exists
    static const int Type = (sizeof(Check<ValueType, ArchiveType>(0)) == sizeof(long long) ? Primitive :
                             sizeof(Check<ValueType, ArchiveType>(0)) == sizeof(long) ? Member :
                             sizeof(Check<ValueType, ArchiveType>(0)) == sizeof(short) ? Global :
                             None);
};

/** Helper for determining the type used by a given abstract object class (i.e. the type returned by the Type() function) */
#define ABSTRACT_TYPE decltype( std::declval<ValType>().Type() )

/** For abstract types, determine what kind of ArchiveConstructor the type has */
template<typename ValType, class ArchiveType>
struct ArchiveConstructorType
{
    typedef ABSTRACT_TYPE ObjType;

    enum { Basic, Advanced, None };

    // Check for ValueType::ArchiveConstructor(ObjectType, const IArchive&)
    template<typename ValueType, typename ObjectType> static long&   Check(FunctionExists<ValueType* (*)(ObjectType, const ArchiveType&), &ValueType::ArchiveConstructor>*);
    // Check for ValueType::ArchiveConstructor(ObjectType)
    template<typename ValueType, typename ObjectType> static short&  Check(FunctionExists<ValueType* (*)(ObjectType), &ValueType::ArchiveConstructor>*);
    // Fallback - no valid ArchiveConstructor method exists
    template<typename ValueType, typename ObjectType> static char&   Check(...);

    // Set Type enum to correspond to whichever function exists
    static const int Type = (sizeof(Check<ValType, ObjType>(0)) == sizeof(long) ? Advanced :
                             sizeof(Check<ValType, ObjType>(0)) == sizeof(short) ? Basic :
                             None);

    // If you fail here, you are missing an ArchiveConstructor() function, or you do have one but it is malformed.
    // Check the comments at the top of this source file for details on serialization requirements for abstract objects.
    static_assert(Type != None, "Abstract objects being serialized must have virtual Type() and static ArchiveConstructor() functions.");
};

/** Helper that turns functions on or off depending on their serialize type */
#define IS_SERIAL_TYPE(SType) (SerialType<ValType, IArchive>::Type == SerialType<ValType, IArchive>::##SType)

/** Helper that turns functions on or off depending on their StaticConstructor type */
#define IS_ARCHIVE_CONSTRUCTOR_TYPE(CType) (ArchiveConstructorType<ValType, IArchive>::Type == ArchiveConstructorType<ValType, IArchive>::##CType)

/** Helper that turns functions on or off depending on if the parameter type is abstract */
#define IS_ABSTRACT std::is_abstract<ValType>::value

/** IArchive - Main serializer archive interface */
class IArchive
{
protected:
    u16 mArchiveVersion;
    u16 mFileVersion;
    EGame mGame;

    // Subclasses must fill in flags in their constructors!!!
    u32 mArchiveFlags;

    // Info about the stack of parameters being serialized
    struct SParmStackEntry
    {
        size_t TypeID;
        size_t TypeSize;
        void* pDataPointer;
        u32 HintFlags;
    };
    std::vector<SParmStackEntry> mParmStack;

public:
    enum EArchiveVersion
    {
        eArVer_Initial,
        eArVer_32BitBinarySize,
        eArVer_Refactor,
        // Insert new versions before this line
        eArVer_Max
    };
    static const u32 skCurrentArchiveVersion = (eArVer_Max - 1);

    IArchive()
        : mFileVersion(0)
        , mArchiveVersion(skCurrentArchiveVersion)
        , mGame(eUnknownGame)
        , mArchiveFlags(0)
    {
        // hack to reduce allocations
        mParmStack.reserve(16);
    }

    virtual ~IArchive() {}

protected:
    // Serialize archive version. Always call after opening a file.
    void SerializeVersion()
    {
        *this << SerialParameter("ArchiveVer",  mArchiveVersion,    SH_Attribute)
              << SerialParameter("FileVer",     mFileVersion,       SH_Attribute | SH_Optional,     (u16) 0)
              << SerialParameter("Game",        mGame,              SH_Attribute | SH_Optional,     eUnknownGame);
    }

    // Return whether this parameter should be serialized
    template<typename ValType>
    bool ShouldSerializeParameter(const TSerialParameter<ValType>& Param)
    {
        if (mArchiveFlags & AF_NoSkipping)
            return true;

        if ( IsWriter() )
        {
            if (Param.HintFlags & SH_NeverSave)
                return false;

            if ( ( Param.HintFlags & SH_Optional ) &&
                 ( Param.HintFlags & SH_AlwaysSave ) == 0 &&
                 ( ParameterMatchesDefault(Param) ) )
                return false;
        }

        return true;
    }

    // Instantiate an abstract object from the file
    // Only readers are allowed to instantiate objects
    template<typename ValType, typename ObjType = ABSTRACT_TYPE>
    ENABLE_IF( IS_ARCHIVE_CONSTRUCTOR_TYPE(Basic), ValType* )
    inline InstantiateAbstractObject(const TSerialParameter<ValType*>& Param, ObjType Type)
    {
        // Variant for basic static constructor
        ASSERT( IsReader() );
        return ValType::ArchiveConstructor(Type);
    }

    template<typename ValType, typename ObjType = ABSTRACT_TYPE>
    ENABLE_IF( IS_ARCHIVE_CONSTRUCTOR_TYPE(Advanced), ValType* )
    InstantiateAbstractObject(const TSerialParameter<ValType*>& Param, ObjType Type)
    {
        // Variant for advanced static constructor
        ASSERT( IsReader() );
        return ValType::ArchiveConstructor(Type, *this);
    }

    // Parameter stack handling
    template<typename ValType>
    inline void PushParameter(const TSerialParameter<ValType>& Param)
    {
#if _DEBUG
        if (mParmStack.size() > 0)
        {
            // Attribute properties cannot have children!
            ASSERT( (mParmStack.back().HintFlags & SH_Attribute) == 0 );
        }
#endif

        SParmStackEntry Entry;
        Entry.TypeID = typeid(ValType).hash_code();
        Entry.TypeSize = sizeof(ValType);
        Entry.pDataPointer = &Param.rValue;
        Entry.HintFlags = Param.HintFlags;
        mParmStack.push_back(Entry);
    }

    template<typename ValType>
    inline void PopParameter(const TSerialParameter<ValType>& Param)
    {
#if _DEBUG
        // Make sure the entry matches the param that has been passed in
        ASSERT(mParmStack.size() > 0);
        const SParmStackEntry& kEntry = mParmStack.back();
        ASSERT(kEntry.TypeID == typeid(ValType).hash_code());
        ASSERT(kEntry.pDataPointer == &Param.rValue);
#endif
        mParmStack.pop_back();
    }

public:
    // Serialize primitives
    template<typename ValType>
    ENABLE_IF( IS_SERIAL_TYPE(Primitive), IArchive& )
    operator<<(TSerialParameter<ValType> rParam)
    {
        PushParameter(rParam);

        if (ShouldSerializeParameter(rParam)
            && ParamBegin(rParam.pkName, rParam.HintFlags))
        {
            SerializePrimitive(rParam.rValue, rParam.HintFlags);
            ParamEnd();
        }
        else if (IsReader())
            InitParameterToDefault(rParam);

        PopParameter(rParam);
        return *this;
    }

    template<typename ValType>
    ENABLE_IF( IS_SERIAL_TYPE(Primitive) && !IS_ABSTRACT, IArchive& )
    operator<<(TSerialParameter<ValType*> rParam)
    {
        ASSERT( !(mArchiveFlags & AF_Writer) || rParam.rValue != nullptr );
        PushParameter(rParam);

        if (ShouldSerializeParameter(rParam)
            && ParamBegin(rParam.pkName, rParam.HintFlags))
        {
            // Support for old versions of archives that serialize types on non-abstract polymorphic pointers
            if (ArchiveVersion() < eArVer_Refactor && IsReader() && std::is_polymorphic_v<ValType>)
            {
                u32 Type;
                *this << SerialParameter("Type", Type, SH_Attribute);
            }

            if (PreSerializePointer(rParam.rValue, rParam.HintFlags))
            {
                if (rParam.rValue == nullptr && (mArchiveFlags & AF_Reader))
                    rParam.rValue = new ValType;

                if (rParam.rValue != nullptr)
                    SerializePrimitive(*rParam.rValue, rParam.HintFlags);
            }
            else if (IsReader())
                rParam.rValue = nullptr;

            ParamEnd();
        }

        PopParameter(rParam);
        return *this;
    }

    // Serialize objects with global Serialize functions
    template<typename ValType>
    ENABLE_IF( IS_SERIAL_TYPE(Global), IArchive& )
    inline operator<<(TSerialParameter<ValType> rParam)
    {
        PushParameter(rParam);

        if (ShouldSerializeParameter(rParam)
            && ParamBegin(rParam.pkName, rParam.HintFlags))
        {
            Serialize(*this, rParam.rValue);
            ParamEnd();
        }
        else if (IsReader())
            InitParameterToDefault(rParam);

        PopParameter(rParam);
        return *this;
    }

    template<typename ValType>
    ENABLE_IF( IS_SERIAL_TYPE(Global) && !IS_ABSTRACT, IArchive&)
    operator<<(TSerialParameter<ValType*> rParam)
    {
        ASSERT( !IsWriter() || rParam.rValue != nullptr );
        PushParameter(rParam);

        if (ShouldSerializeParameter(rParam)
            && ParamBegin(rParam.pkName))
        {
            // Support for old versions of archives that serialize types on non-abstract polymorphic pointers
            if (ArchiveVersion() < eArVer_Refactor && IsReader() && std::is_polymorphic_v<ValType>)
            {
                u32 Type;
                *this << SerialParameter("Type", Type, SH_Attribute);
            }

            if (PreSerializePointer(rParam.rValue, rParam.HintFlags, rParam.HintFlags))
            {
                if (rParam.rValue == nullptr && IsReader())
                    rParam.rValue = new ValType;

                if (rParam.rValue != nullptr)
                    Serialize(*this, *rParam.rValue);
            }
            else if (IsReader())
                rParam.rValue = nullptr;

            ParamEnd();
        }

        PopParameter(rParam);
        return *this;
    }

    // Serialize objects with Serialize methods
    template<typename ValType>
    ENABLE_IF( IS_SERIAL_TYPE(Member), IArchive& )
    operator<<(TSerialParameter<ValType> rParam)
    {
        PushParameter(rParam);

        if (ShouldSerializeParameter(rParam) &&
            ParamBegin(rParam.pkName, rParam.HintFlags))
        {
            rParam.rValue.Serialize(*this);
            ParamEnd();
        }
        else if (IsReader())
            InitParameterToDefault(rParam);

        PopParameter(rParam);
        return *this;
    }

    template<typename ValType>
    ENABLE_IF( IS_SERIAL_TYPE(Member) && !IS_ABSTRACT, IArchive& )
    operator<<(TSerialParameter<ValType*> rParam)
    {
        PushParameter(rParam);

        if (ShouldSerializeParameter(rParam) &&
            ParamBegin(rParam.pkName, rParam.HintFlags))
        {
            // Support for old versions of archives that serialize types on non-abstract polymorphic pointers
            if (ArchiveVersion() < eArVer_Refactor && IsReader() && std::is_polymorphic_v<ValType>)
            {
                u32 Type;
                *this << SerialParameter("Type", Type, SH_Attribute);
            }

            if (PreSerializePointer((void*&) rParam.rValue, rParam.HintFlags))
            {
                if (rParam.rValue == nullptr && IsReader())
                    rParam.rValue = new ValType;

                if (rParam.rValue != nullptr)
                    rParam.rValue->Serialize(*this);
            }
            else if (IsReader())
                rParam.rValue = nullptr;

            ParamEnd();
        }

        PopParameter(rParam);
        return *this;
    }

    // Serialize polymorphic objects
    template<typename ValType, typename ObjectType = decltype( std::declval<ValType>().Type() )>
    ENABLE_IF( IS_SERIAL_TYPE(Member) && IS_ABSTRACT, IArchive&)
    operator<<(TSerialParameter<ValType*> rParam)
    {
        PushParameter(rParam);

        if (ShouldSerializeParameter(rParam) &&
            ParamBegin(rParam.pkName, rParam.HintFlags))
        {
            if (PreSerializePointer( (void*&) rParam.rValue, rParam.HintFlags ))
            {
                // Non-reader archives cannot instantiate the class. It must exist already.
                if (IsWriter())
                {
                    ObjectType Type = rParam.rValue->Type();
                    *this << SerialParameter("Type", Type, SH_Attribute);
                }
                else
                {
                    // NOTE: If you crash here, it likely means that the pointer was initialized to a garbage value.
                    // It is legal to serialize a pointer that already exists, so you still need to initialize it.
                    ObjectType Type = (rParam.rValue ? rParam.rValue->Type() : ObjectType());
                    ObjectType TypeCopy = Type;
                    *this << SerialParameter("Type", Type, SH_Attribute);

                    if (IsReader() && rParam.rValue == nullptr)
                    {
                        rParam.rValue = InstantiateAbstractObject(rParam, Type);
                    }
                    else if (rParam.rValue != nullptr)
                    {
                        // Make sure the type is what we are expecting
                        ASSERT(Type == TypeCopy);
                    }
                }

                // At this point, the object should exist and is ready for serializing.
                if (rParam.rValue)
                {
                    rParam.rValue->Serialize(*this);
                }
            }
            else if (IsReader())
                rParam.rValue = nullptr;

            ParamEnd();
        }
        else
        {
            // Polymorphic types don't support default values
        }

        PopParameter(rParam);
        return *this;
    }

    // Error
    template<typename ValType>
    ENABLE_IF( IS_SERIAL_TYPE(Global) && IS_ABSTRACT, IArchive& )
    operator<<(TSerialParameter<ValType*>)
    {
        static_assert(false, "Global Serialize method for polymorphic type pointers is not supported.");
    }

    // Generate compiler errors for classes with no valid Serialize function defined
    template<typename ValType>
    ENABLE_IF( IS_SERIAL_TYPE(None), IArchive& )
    operator<<(TSerialParameter<ValType>)
    {
        static_assert(false, "Object being serialized has no valid Serialize method defined.");
    }

    // Interface
    virtual bool ParamBegin(const char *pkName, u32 Flags) = 0;
    virtual void ParamEnd() = 0;

    virtual bool PreSerializePointer(void*& Pointer, u32 Flags) = 0;
    virtual void SerializePrimitive(bool& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(char& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(s8& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(u8& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(s16& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(u16& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(s32& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(u32& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(s64& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(u64& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(float& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(double& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(TString& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(TWideString& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(CFourCC& rValue, u32 Flags) = 0;
    virtual void SerializePrimitive(CAssetID& rValue, u32 Flags) = 0;
    virtual void SerializeBulkData(void* pData, u32 DataSize, u32 Flags) = 0;

    // Optional - serialize in an array size. By default, just stores size as an attribute property.
    virtual void SerializeArraySize(u32& Value)
    {
        *this << SerialParameter("Size", Value, SH_Attribute);
    }

    // Accessors
    inline bool IsReader() const                { return (mArchiveFlags & AF_Reader) != 0; }
    inline bool IsWriter() const                { return (mArchiveFlags & AF_Writer) != 0; }
    inline bool IsTextFormat() const            { return (mArchiveFlags & AF_Text) != 0; }
    inline bool IsBinaryFormat() const          { return (mArchiveFlags & AF_Binary) != 0; }

    inline u16 ArchiveVersion() const   { return mArchiveVersion; }
    inline u16 FileVersion() const      { return mFileVersion; }
    inline EGame Game() const           { return mGame; }

    inline void SetVersion(u16 ArchiveVersion, u16 FileVersion, EGame Game)
    {
        mArchiveVersion = ArchiveVersion;
        mFileVersion = FileVersion;
        mGame = Game;
    }

    inline void SetVersion(const CSerialVersion& rkVersion)
    {
        mArchiveVersion = rkVersion.ArchiveVersion();
        mFileVersion = rkVersion.FileVersion();
        mGame = rkVersion.Game();
    }

    inline CSerialVersion GetVersionInfo() const
    {
        return CSerialVersion(mArchiveVersion, mFileVersion, mGame);
    }

    /** Returns the last object of a requested type in the parameter stack. Returns nullptr if the wasn't one.
     *  This function will not return the current object being serialized. */
    template<typename ValType>
    ValType* FindParentObject() const
    {
        for (int ParmIdx = mParmStack.size() - 2; ParmIdx >= 0; ParmIdx--)
        {
            const SParmStackEntry& kEntry = mParmStack[ParmIdx];

            if (kEntry.TypeID == typeid(ValType).hash_code())
            {
                return static_cast<ValType*>(kEntry.pDataPointer);
            }
        }

        return nullptr;
    }
};

#if WITH_CODEGEN
// Default enum serializer; can be overridden
#include <codegen/EnumReflection.h>

template<typename T, typename = typename std::enable_if<std::is_enum<T>::value>::type>
inline void Serialize(IArchive& Arc, T& Val)
{
    if (Arc.IsTextFormat())
    {
        if (Arc.IsReader())
        {
            TString ValueName;
            Arc.SerializePrimitive(ValueName, 0);
            Val = TEnumReflection<T>::ConvertStringToValue( *ValueName );
        }
        else
        {
            TString ValueName = TEnumReflection<T>::ConvertValueToString(Val);
            Arc.SerializePrimitive(ValueName, 0);
        }
    }
    else
    {
        Arc.SerializePrimitive((u32&) Val, 0);
    }
}
#endif

// Container serialize methods

// std::vector
template<typename T>
inline void Serialize(IArchive& Arc, std::vector<T>& Vector)
{
    u32 Size = Vector.size();
    Arc.SerializeArraySize(Size);

    if (Arc.IsReader())
    {
        Vector.resize(Size);
    }

    for (u32 i = 0; i < Size; i++)
    {
        // SH_IgnoreName to preserve compatibility with older files that may have differently-named items
        Arc << SerialParameter("Item", Vector[i], SH_IgnoreName);
    }
}

// overload for std::vector<u8> that serializes in bulk
template<>
inline void Serialize(IArchive& Arc, std::vector<u8>& Vector)
{
    // Don't use SerializeArraySize, bulk data is a special case that overloads may not handle correctly
    u32 Size = Vector.size();
    Arc << SerialParameter("Size", Size, SH_Attribute);

    if (Arc.IsReader())
    {
        Vector.resize(Size);
    }

    Arc.SerializeBulkData(Vector.data(), Vector.size(), 0);
}

// std::list
template<typename T>
inline void Serialize(IArchive& Arc, std::list<T>& List)
{
    u32 Size = List.size();
    Arc.SerializeArraySize(Size);

    if (Arc.IsReader())
    {
        List.resize(Size);
    }

    for (auto Iter = List.begin(); Iter != List.end(); Iter++)
        Arc << SerialParameter("Item", *Iter, SH_IgnoreName);
}

// Overload for TStringList and TWideStringList so they can use the TString/TWideString serialize functions
inline void Serialize(IArchive& Arc, TStringList& List)
{
    std::list<TString>& GenericList = *reinterpret_cast< std::list<TString>* >(&List);
    Serialize(Arc, GenericList);
}

inline void Serialize(IArchive& Arc, TWideStringList& List)
{
    std::list<TWideString>& GenericList = *reinterpret_cast< std::list<TWideString>* >(&List);
    Serialize(Arc, GenericList);
}

// std::set
template<typename T>
inline void Serialize(IArchive& Arc, std::set<T>& Set)
{
    u32 Size = Set.size();
    Arc.SerializeArraySize(Size);

    if (Arc.IsReader())
    {
        for (u32 i = 0; i < Size; i++)
        {
            T Val;
            Arc << SerialParameter("Item", Val, SH_IgnoreName);
            Set.insert(Val);
        }
    }

    else
    {
        for (auto Iter = Set.begin(); Iter != Set.end(); Iter++)
        {
            T Val = *Iter;
            Arc << SerialParameter("Item", Val, SH_IgnoreName);
        }
    }
}

// std::map and std::unordered_map
template<typename KeyType, typename ValType, typename MapType>
inline void SerializeMap_Internal(IArchive& Arc, MapType& Map)
{
    u32 Size = Map.size();
    Arc.SerializeArraySize(Size);

    if (Arc.IsReader())
    {
        for (u32 i = 0; i < Size; i++)
        {
            KeyType Key;
            ValType Val;

            if (Arc.ParamBegin("Item", SH_IgnoreName))
            {
                Arc << SerialParameter("Key", Key, SH_IgnoreName)
                    << SerialParameter("Value", Val, SH_IgnoreName);

                ASSERT(Map.find(Key) == Map.end());
                Map[Key] = Val;
                Arc.ParamEnd();
            }
        }
    }

    else
    {
        for (auto Iter = Map.begin(); Iter != Map.end(); Iter++)
        {
            // Creating copies is not ideal, but necessary because parameters cannot be const.
            // Maybe this can be avoided somehow?
            KeyType Key = Iter->first;
            ValType Val = Iter->second;

            if (Arc.ParamBegin("Item", SH_IgnoreName))
            {
                Arc << SerialParameter("Key", Key, SH_IgnoreName)
                    << SerialParameter("Value", Val, SH_IgnoreName);

                Arc.ParamEnd();
            }
        }
    }
}

template<typename KeyType, typename ValType>
inline void Serialize(IArchive& Arc, std::map<KeyType, ValType>& Map)
{
    SerializeMap_Internal<KeyType, ValType, std::map<KeyType, ValType> >(Arc, Map);
}

template<typename KeyType, typename ValType>
inline void Serialize(IArchive& Arc, std::unordered_map<KeyType, ValType>& Map)
{
    SerializeMap_Internal<KeyType, ValType, std::unordered_map<KeyType, ValType> >(Arc, Map);
}

// Remove header-only macros
#undef ENABLE_IF
#undef SUPPORTS_DEFAULT_VALUES
#undef IS_ABSTRACT
#undef IS_SERIAL_TYPE
#undef IS_ARCHIVE_CONSTRUCTOR_TYPE
#undef ABSTRACT_TYPE

#endif // IARCHIVE
