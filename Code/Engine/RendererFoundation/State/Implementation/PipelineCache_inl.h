
template <typename HandleType, typename DescType, typename KeyType>
HandleType WGALPipelineCache::TryGetPipeline(const DescType& description, WHashTable<KeyType, HandleType, WGALPipelineCache::CacheKeyHasher>& table)
{
  W_ASSERT_DEV(m_pDevice != nullptr, "GAL device not initialized");

  KeyType key;
  key.m_Desc = description;
  key.m_uiHash = description.CalculateHash();

  {
    W_LOCK(m_Mutex);

    HandleType* pExistingPipeline = table.GetValue(key);
    if (pExistingPipeline != nullptr)
    {
      return *pExistingPipeline;
    }
  }
  return {};
}

template <typename HandleType, typename DescType, typename KeyType>
W_ALWAYS_INLINE WResult WGALPipelineCache::TryInsertPipeline(const DescType& description, HandleType hNewPipeline, WHashTable<KeyType, HandleType, WGALPipelineCache::CacheKeyHasher>& table)
{
  KeyType key;
  key.m_Desc = description;
  key.m_uiHash = description.CalculateHash();

  W_LOCK(m_Mutex);

  HandleType existingPipeline;
  if (table.Insert(key, hNewPipeline, &existingPipeline))
  {
    W_ASSERT_DEBUG(existingPipeline == hNewPipeline, "On collision, both pipelines must be the same (create should have just increased the ref count)");
    return W_FAILURE;
  }
  return W_SUCCESS;
}
