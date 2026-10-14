#ifdef DZ_Expansion_Navigation
modded class ExpansionMarkerClientData
{
	ExpansionMarkerClientInfo ZenNoBuildZoneInfo()
	{
		string uid = ZenCircleMapMarkerConstants.NO_BUILD_ZONE_CATEGORY_ID;
		ExpansionMarkerClientInfo info = m_MarkerInfo_Server.Get(uid);
		if (!info)
		{
			info = new ExpansionMarkerClientInfo(uid);
			info.ApplyVisibility(EXPANSION_MARKER_VIS_NONE);
			m_MarkerInfo_Server.Insert(uid, info);
		}
		return info;
	}

	override void OnRefresh()
	{
		// Expansion prunes visibility entries without a corresponding map pin.
		// Keep a strong reference while it is removed, then restore only our category.
		ref ExpansionMarkerClientInfo info = m_MarkerInfo_Server.Get(ZenCircleMapMarkerConstants.NO_BUILD_ZONE_CATEGORY_ID);
		super.OnRefresh();
		if (info)
			m_MarkerInfo_Server.Set(ZenCircleMapMarkerConstants.NO_BUILD_ZONE_CATEGORY_ID, info);
	}
};

class ZenCircleMapMarkerNoBuildVisibility
{
	static bool IsVisible(bool respectServerVisibility)
	{
		ExpansionMarkerModule module;
		if (!CF_Modules<ExpansionMarkerModule>.Get(module) || !module.GetData())
			return false;

		if (!module.GetData().ZenNoBuildZoneInfo().IsMapVisible())
			return false;

		return !respectServerVisibility || module.IsMapVisible(ExpansionMapMarkerType.SERVER);
	}
};
#endif
