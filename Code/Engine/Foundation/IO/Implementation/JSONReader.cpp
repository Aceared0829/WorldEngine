#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONReader.h>


WJSONReader::WJSONReader()
{
  m_bParsingError = false;
}

WResult WJSONReader::Parse(WStreamReader& ref_inputStream, WUInt32 uiFirstLineOffset)
{
  m_bParsingError = false;
  m_Stack.Clear();
  m_sLastName.Clear();

  SetInputStream(ref_inputStream, uiFirstLineOffset);

  while (!m_bParsingError && ContinueParsing())
  {
  }

  if (m_bParsingError)
  {
    m_Stack.Clear();
    m_Stack.PushBack(Element());

    return W_FAILURE;
  }

  // make sure there is one top level element
  if (m_Stack.IsEmpty())
  {
    Element& e = m_Stack.ExpandAndGetRef();
    e.m_Mode = ElementType::None;
  }

  return W_SUCCESS;
}

bool WJSONReader::OnVariable(WStringView sVarName)
{
  m_sLastName = sVarName;

  return true;
}

void WJSONReader::OnReadValue(WStringView sValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(std::move(WString(sValue)));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = std::move(WString(sValue));

  m_sLastName.Clear();
}

void WJSONReader::OnReadValue(double fValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(WVariant(fValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = WVariant(fValue);

  m_sLastName.Clear();
}

void WJSONReader::OnReadValue(bool bValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(WVariant(bValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = WVariant(bValue);

  m_sLastName.Clear();
}

void WJSONReader::OnReadValueNULL()
{
  if (m_Stack.PeekBack().m_Mode == ElementType::Array)
    m_Stack.PeekBack().m_Array.PushBack(WVariant());
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = WVariant();

  m_sLastName.Clear();
}

void WJSONReader::OnBeginObject()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode = ElementType::Dictionary;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void WJSONReader::OnEndObject()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementType::Array)
    {
      Parent.m_Array.PushBack(Child.m_Dictionary);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = std::move(Child.m_Dictionary);
    }

    m_Stack.PopBack();
  }
  else
  {
    // do nothing, keep the top-level dictionary
  }
}

void WJSONReader::OnBeginArray()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode = ElementType::Array;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void WJSONReader::OnEndArray()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementType::Array)
    {
      Parent.m_Array.PushBack(Child.m_Array);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = std::move(Child.m_Array);
    }

    m_Stack.PopBack();
  }
  else
  {
    // do nothing, keep the top-level array
  }
}

void WJSONReader::OnParsingError(WStringView sMessage, bool bFatal, WUInt32 uiLine, WUInt32 uiColumn)
{
  W_IGNORE_UNUSED(sMessage);
  W_IGNORE_UNUSED(bFatal);
  W_IGNORE_UNUSED(uiLine);
  W_IGNORE_UNUSED(uiColumn);

  m_bParsingError = true;
}
