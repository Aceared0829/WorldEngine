
template <typename T>
WCustomDataResource<T>::WCustomDataResource() = default;

template <typename T>
WCustomDataResource<T>::~WCustomDataResource() = default;

template <typename T>
void WCustomDataResource<T>::CreateAndLoadData(WAbstractObjectGraph& ref_graph, WRttiConverterContext& ref_context, const WAbstractObjectNode* pRootNode)
{
  T* pData = reinterpret_cast<T*>(m_Data);

  if (GetLoadingState() == WResourceState::Loaded)
  {
    WMemoryUtils::Destruct(pData);
  }

  WMemoryUtils::Construct<SkipTrivialTypes>(pData);

  if (pRootNode)
  {
    // pRootNode is empty when the resource file is empty
    // no need to attempt to load it then
    pData->Load(ref_graph, ref_context, pRootNode);
  }
}

template <typename T>
WResourceLoadDesc WCustomDataResource<T>::UnloadData(Unload WhatToUnload)
{
  if (GetData() != nullptr)
  {
    WMemoryUtils::Destruct(GetData());
  }

  return WCustomDataResourceBase::UnloadData(WhatToUnload);
}

template <typename T>
WResourceLoadDesc WCustomDataResource<T>::UpdateContent(WStreamReader* Stream)
{
  return UpdateContent_Internal(Stream, *WGetStaticRTTI<T>());
}

template <typename T>
void WCustomDataResource<T>::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(WCustomDataResource<T>);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}
