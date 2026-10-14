modded class MapMenu
{
	protected ref ZenCircleMapMarkerRenderer m_ZenCircleMapMarkerVanillaRenderer;

	override void OnShow()
	{
		super.OnShow();

		if (!ZenCircleMapMarkers_IsVanillaMapMenuActive())
			return;

		if (!m_ZenCircleMapMarkerVanillaRenderer && m_MapWidgetInstance && GetLayoutRoot())
			m_ZenCircleMapMarkerVanillaRenderer = new ZenCircleMapMarkerRenderer(m_MapWidgetInstance, GetLayoutRoot());

		if (m_ZenCircleMapMarkerVanillaRenderer)
			m_ZenCircleMapMarkerVanillaRenderer.Show();

		ZenCircleMapMarkers_KeepVanillaMapUIAboveCircles();
	}

	override void OnHide()
	{
		if (m_ZenCircleMapMarkerVanillaRenderer)
			m_ZenCircleMapMarkerVanillaRenderer.Hide();

		super.OnHide();
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		if (!ZenCircleMapMarkers_IsVanillaMapMenuActive())
			return;

		if (m_ZenCircleMapMarkerVanillaRenderer)
			m_ZenCircleMapMarkerVanillaRenderer.Update(timeslice);

		ZenCircleMapMarkers_KeepVanillaMapUIAboveCircles();
	}

	protected bool ZenCircleMapMarkers_IsVanillaMapMenuActive()
	{
		UIScriptedMenu vanillaMapMenu = g_Game.GetUIManager().FindMenu(MENU_MAP);
		return vanillaMapMenu && vanillaMapMenu == this;
	}

	protected void ZenCircleMapMarkers_KeepVanillaMapUIAboveCircles()
	{
		if (m_MapToolsContainer && m_MapToolsContainer.GetSort() < ZenCircleMapMarkerConstants.TOOLS_SORT)
			m_MapToolsContainer.SetSort(ZenCircleMapMarkerConstants.TOOLS_SORT);

		if (m_ToolbarPanel && m_ToolbarPanel.GetSort() < ZenCircleMapMarkerConstants.TOOLS_SORT)
			m_ToolbarPanel.SetSort(ZenCircleMapMarkerConstants.TOOLS_SORT);
	}
};
