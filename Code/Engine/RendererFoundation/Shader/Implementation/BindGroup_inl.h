
WGALBindGroupItem::WGALBindGroupItem()
{
}

WGALBindGroupItem::WGALBindGroupItem(const WGALBindGroupItem& rhs)
{
  *this = rhs;
}

void WGALBindGroupItem::operator=(const WGALBindGroupItem& rhs)
{
  WHashableStruct<WGALBindGroupItem>& thisBase = *this;
  const WHashableStruct<WGALBindGroupItem>& rhsBase = rhs;
  thisBase = rhsBase;
}

WGALBindGroup::WGALBindGroup(const WGALBindGroupCreationDescription& Description)
  : WGALResource(Description)
{
}

WGALBindGroup::~WGALBindGroup() = default;
