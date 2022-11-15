// MethodProps.cpp

#include "StdAfx.h"

#include "../../Common/StringToInt.h"

#include "MethodProps.h"

using namespace NWindows;

bool StringToBool(const wchar_t *s, bool &res)
{
  if (s[0] == 0 || (s[0] == '+' && s[1] == 0) || StringsAreEqualNoCase_Ascii(s, "ON"))
  {
    res = true;
    return true;
  }
  if ((s[0] == '-' && s[1] == 0) || StringsAreEqualNoCase_Ascii(s, "OFF"))
  {
    res = false;
    return true;
  }
  return false;
}

HRESULT PROPVARIANT_to_bool(const PROPVARIANT &prop, bool &dest)
{
  switch (prop.vt)
  {
    case VT_EMPTY: dest = true; return S_OK;
    case VT_BOOL: dest = (prop.boolVal != VARIANT_FALSE); return S_OK;
    case VT_BSTR: return StringToBool(prop.bstrVal, dest) ? S_OK : E_INVALIDARG;
  }
  return E_INVALIDARG;
}

unsigned ParseStringToUInt32(const UString &srcString, UInt32 &number)
{
  const wchar_t *start = srcString;
  const wchar_t *end;
  number = ConvertStringToUInt32(start, &end);
  return (unsigned)(end - start);
}

static unsigned ParseStringToUInt64(const UString &srcString, UInt64 &number)
{
  const wchar_t *start = srcString;
  const wchar_t *end;
  number = ConvertStringToUInt64(start, &end);
  return (unsigned)(end - start);
}

HRESULT ParsePropToUInt32(const UString &name, const PROPVARIANT &prop, UInt32 &resValue)
{
  // =VT_UI4
  // =VT_EMPTY
  // {stringUInt32}=VT_EMPTY

  if (prop.vt == VT_UI4)
  {
    if (!name.IsEmpty())
      return E_INVALIDARG;
    resValue = prop.ulVal;
    return S_OK;
  }
  if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  if (name.IsEmpty())
    return S_OK;
  UInt32 v;
  if (ParseStringToUInt32(name, v) != name.Len())
    return E_INVALIDARG;
  resValue = v;
  return S_OK;
}

HRESULT ParseMtProp(const UString &name, const PROPVARIANT &prop, UInt32 defaultNumThreads, UInt32 &numThreads)
{
  if (name.IsEmpty())
  {
    switch (prop.vt)
    {
      case VT_UI4:
        numThreads = prop.ulVal;
        break;
      default:
      {
        bool val;
        RINOK(PROPVARIANT_to_bool(prop, val));
        numThreads = (val ? defaultNumThreads : 1);
        break;
      }
    }
    return S_OK;
  }
  if (prop.vt != VT_EMPTY)
    return E_INVALIDARG;
  return ParsePropToUInt32(name, prop, numThreads);
}


static HRESULT SetLogSizeProp(UInt64 number, NCOM::CPropVariant &destProp)
{
  if (number >= 64)
    return E_INVALIDARG;
  UInt32 val32;
  if (number < 32)
    val32 = (UInt32)1 << (unsigned)number;
  /*
  else if (number == 32 && reduce_4GB_to_32bits)
    val32 = (UInt32)(Int32)-1;
  */
  else
  {
    destProp = (UInt64)((UInt64)1 << (unsigned)number);
    return S_OK;
  }
  destProp = (UInt32)val32;
  return S_OK;
}


static HRESULT StringToDictSize(const UString &s, NCOM::CPropVariant &destProp)
{
  /* if (reduce_4GB_to_32bits) we can reduce (4 GiB) property to (4 GiB - 1).
     to fit the value to UInt32 for clients that do not support 64-bit values */

  const wchar_t *end;
  const UInt64 number = ConvertStringToUInt64(s, &end);
  const unsigned numDigits = (unsigned)(end - s.Ptr());
  if (numDigits == 0 || s.Len() > numDigits + 1)
    return E_INVALIDARG;
  
  if (s.Len() == numDigits)
    return SetLogSizeProp(number, destProp);
  
  unsigned numBits;
  
  switch (MyCharLower_Ascii(s[numDigits]))
  {
    case 'b': numBits =  0; break;
    case 'k': numBits = 10; break;
    case 'm': numBits = 20; break;
    case 'g': numBits = 30; break;
    default: return E_INVALIDARG;
  }
  
  const UInt64 range4g = ((UInt64)1 << (32 - numBits));
  if (number < range4g)
    destProp = (UInt32)((UInt32)number << numBits);
  /*
  else if (number == range4g && reduce_4GB_to_32bits)
    destProp = (UInt32)(Int32)-1;
  */
  else if (numBits == 0)
    destProp = (UInt64)number;
  else if (number >= ((UInt64)1 << (64 - numBits)))
    return E_INVALIDARG;
  else
    destProp = (UInt64)((UInt64)number << numBits);
  return S_OK;
}


static HRESULT PROPVARIANT_to_DictSize(const PROPVARIANT &prop, NCOM::CPropVariant &destProp)
{
  if (prop.vt == VT_UI4)
    return SetLogSizeProp(prop.ulVal, destProp);

  if (prop.vt == VT_BSTR)
  {
    UString s;
    s = prop.bstrVal;
    return StringToDictSize(s, destProp);
  }
  return E_INVALIDARG;
}


void CProps::AddProp32(PROPID propid, UInt32 val)
{
  CProp &prop = Props.AddNew();
  prop.IsOptional = true;
  prop.Id = propid;
  prop.Value = (UInt32)val;
}

void CProps::AddPropBool(PROPID propid, bool val)
{
  CProp &prop = Props.AddNew();
  prop.IsOptional = true;
  prop.Id = propid;
  prop.Value = val;
}

class CCoderProps
{
  PROPID *_propIDs;
  NCOM::CPropVariant *_props;
  unsigned _numProps;
  unsigned _numPropsMax;
public:
  CCoderProps(unsigned numPropsMax):
      _propIDs(NULL),
      _props(NULL),
      _numProps(0),
      _numPropsMax(numPropsMax)
  {
    _propIDs = new PROPID[numPropsMax];
    _props = new NCOM::CPropVariant[numPropsMax];
  }
  ~CCoderProps()
  {
    delete []_propIDs;
    delete []_props;
  }
  void AddProp(const CProp &prop);
  HRESULT SetProps(ICompressSetCoderProperties *setCoderProperties)
  {
    return setCoderProperties->SetCoderProperties(_propIDs, _props, _numProps);
  }
};

void CCoderProps::AddProp(const CProp &prop)
{
  if (_numProps >= _numPropsMax)
    throw 1;
  _propIDs[_numProps] = prop.Id;
  _props[_numProps] = prop.Value;
  _numProps++;
}

HRESULT CProps::SetCoderProps(ICompressSetCoderProperties *scp, const UInt64 *dataSizeReduce) const
{
  return SetCoderProps_DSReduce_Aff(scp, dataSizeReduce, NULL);
}

HRESULT CProps::SetCoderProps_DSReduce_Aff(
    ICompressSetCoderProperties *scp,
    const UInt64 *dataSizeReduce,
    const UInt64 *affinity) const
{
  CCoderProps coderProps(Props.Size() + (dataSizeReduce ? 1 : 0) + (affinity ? 1 : 0) );
  FOR_VECTOR (i, Props)
    coderProps.AddProp(Props[i]);
  if (dataSizeReduce)
  {
    CProp prop;
    prop.Id = NCoderPropID::kReduceSize;
    prop.Value = *dataSizeReduce;
    coderProps.AddProp(prop);
  }
  if (affinity)
  {
    CProp prop;
    prop.Id = NCoderPropID::kAffinity;
    prop.Value = *affinity;
    coderProps.AddProp(prop);
  }
  return coderProps.SetProps(scp);
}


int CMethodProps::FindProp(PROPID id) const
{
  for (int i = (int)Props.Size() - 1; i >= 0; i--)
    if (Props[(unsigned)i].Id == id)
      return i;
  return -1;
}

unsigned CMethodProps::GetLevel() const
{
  int i = FindProp(NCoderPropID::kLevel);
  if (i < 0)
    return 5;
  if (Props[(unsigned)i].Value.vt != VT_UI4)
    return 9;
  UInt32 level = Props[(unsigned)i].Value.ulVal;
  return level > 9 ? 9 : (unsigned)level;
}

struct CNameToPropID
{
  VARTYPE VarType;
  const char *Name;
};


// the following are related to NCoderPropID::EEnum values

static const CNameToPropID g_NameToPropID[] =
{
  { VT_UI4, "" },
  { VT_UI4, "d" },
  { VT_UI4, "mem" },
  { VT_UI4, "o" },
  { VT_UI4, "c" },
  { VT_UI4, "pb" },
  { VT_UI4, "lc" },
  { VT_UI4, "lp" },
  { VT_UI4, "fb" },
  { VT_BSTR, "mf" },
  { VT_UI4, "mc" },
  { VT_UI4, "pass" },
  { VT_UI4, "a" },
  { VT_UI4, "mt" },
  { VT_BOOL, "eos" },
  { VT_UI4, "x" },
  { VT_UI8, "reduce" },
  { VT_UI8, "expect" },
  { VT_UI4, "b" },
  { VT_UI4, "check" },
  { VT_BSTR, "filter" },
  { VT_UI8, "memuse" }
};

static int FindPropIdExact(const UString &name)
{
  for (unsigned i = 0; i < ARRAY_SIZE(g_NameToPropID); i++)
    if (StringsAreEqualNoCase_Ascii(name, g_NameToPropID[i].Name))
      return (int)i;
  return -1;
}

static bool ConvertProperty(const PROPVARIANT &srcProp, VARTYPE varType, NCOM::CPropVariant &destProp)
{
  if (varType == srcProp.vt)
  {
    destProp = srcProp;
    return true;
  }

  if (varType == VT_UI8 && srcProp.vt == VT_UI4)
  {
    destProp = (UInt64)srcProp.ulVal;
    return true;
  }

  if (varType == VT_BOOL)
  {
    bool res;
    if (PROPVARIANT_to_bool(srcProp, res) != S_OK)
      return false;
    destProp = res;
    return true;
  }
  if (srcProp.vt == VT_EMPTY)
  {
    destProp = srcProp;
    return true;
  }
  return false;
}
    
static void SplitParams(const UString &srcString, UStringVector &subStrings)
{
  subStrings.Clear();
  UString s;
  unsigned len = srcString.Len();
  if (len == 0)
    return;
  for (unsigned i = 0; i < len; i++)
  {
    wchar_t c = srcString[i];
    if (c == L':')
    {
      subStrings.Add(s);
      s.Empty();
    }
    else
      s += c;
  }
  subStrings.Add(s);
}

static void SplitParam(const UString &param, UString &name, UString &value)
{
  int eqPos = param.Find(L'=');
  if (eqPos >= 0)
  {
    name.SetFrom(param, (unsigned)eqPos);
    value = param.Ptr((unsigned)(eqPos + 1));
    return;
  }
  unsigned i;
  for (i = 0; i < param.Len(); i++)
  {
    wchar_t c = param[i];
    if (c >= L'0' && c <= L'9')
      break;
  }
  name.SetFrom(param, i);
  value = param.Ptr(i);
}

static bool IsLogSizeProp(PROPID propid)
{
  switch (propid)
  {
    case NCoderPropID::kDictionarySize:
    case NCoderPropID::kUsedMemorySize:
    case NCoderPropID::kBlockSize:
    case NCoderPropID::kBlockSize2:
    // case NCoderPropID::kReduceSize:
      return true;
  }
  return false;
}

HRESULT CMethodProps::SetParam(const UString &name, const UString &value)
{
  int index = FindPropIdExact(name);
  if (index < 0)
    return E_INVALIDARG;
  const CNameToPropID &nameToPropID = g_NameToPropID[(unsigned)index];
  CProp prop;
  prop.Id = (unsigned)index;

  if (IsLogSizeProp(prop.Id))
  {
    RINOK(StringToDictSize(value, prop.Value));
  }
  else
  {
    NCOM::CPropVariant propValue;
    if (nameToPropID.VarType == VT_BSTR)
      propValue = value;
    else if (nameToPropID.VarType == VT_BOOL)
    {
      bool res;
      if (!StringToBool(value, res))
        return E_INVALIDARG;
      propValue = res;
    }
    else if (!value.IsEmpty())
    {
      if (nameToPropID.VarType == VT_UI4)
      {
        UInt32 number;
        if (ParseStringToUInt32(value, number) == value.Len())
          propValue = number;
        else
          propValue = value;
      }
      else if (nameToPropID.VarType == VT_UI8)
      {
        UInt64 number;
        if (ParseStringToUInt64(value, number) == value.Len())
          propValue = number;
        else
          propValue = value;
      }
      else
        propValue = value;
    }
    if (!ConvertProperty(propValue, nameToPropID.VarType, prop.Value))
      return E_INVALIDARG;
  }
  Props.Add(prop);
  return S_OK;
}

HRESULT CMethodProps::ParseParamsFromString(const UString &srcString)
{
  UStringVector params;
  SplitParams(srcString, params);
  FOR_VECTOR (i, params)
  {
    const UString &param = params[i];
    UString name, value;
    SplitParam(param, name, value);
    RINOK(SetParam(name, value));
  }
  return S_OK;
}

HRESULT CMethodProps::ParseParamsFromPROPVARIANT(const UString &realName, const PROPVARIANT &value)
{
  if (realName.Len() == 0)
  {
    // [empty]=method
    return E_INVALIDARG;
  }
  if (value.vt == VT_EMPTY)
  {
    // {realName}=[empty]
    UString name, valueStr;
    SplitParam(realName, name, valueStr);
    return SetParam(name, valueStr);
  }
  
  // {realName}=value
  int index = FindPropIdExact(realName);
  if (index < 0)
    return E_INVALIDARG;
  const CNameToPropID &nameToPropID = g_NameToPropID[(unsigned)index];
  CProp prop;
  prop.Id = (unsigned)index;
  
  if (IsLogSizeProp(prop.Id))
  {
    RINOK(PROPVARIANT_to_DictSize(value, prop.Value));
  }
  else
  {
    if (!ConvertProperty(value, nameToPropID.VarType, prop.Value))
      return E_INVALIDARG;
  }
  Props.Add(prop);
  return S_OK;
}

HRESULT COneMethodInfo::ParseMethodFromString(const UString &s)
{
  MethodName.Empty();
  int splitPos = s.Find(L':');
  {
    UString temp = s;
    if (splitPos >= 0)
      temp.DeleteFrom((unsigned)splitPos);
    if (!temp.IsAscii())
      return E_INVALIDARG;
    MethodName.SetFromWStr_if_Ascii(temp);
  }
  if (splitPos < 0)
    return S_OK;
  PropsString = s.Ptr((unsigned)(splitPos + 1));
  return ParseParamsFromString(PropsString);
}

HRESULT COneMethodInfo::ParseMethodFromPROPVARIANT(const UString &realName, const PROPVARIANT &value)
{
  if (!realName.IsEmpty() && !StringsAreEqualNoCase_Ascii(realName, "m"))
    return ParseParamsFromPROPVARIANT(realName, value);
  // -m{N}=method
  if (value.vt != VT_BSTR)
    return E_INVALIDARG;
  UString s;
  s = value.bstrVal;
  return ParseMethodFromString(s);
}
