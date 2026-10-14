#ifdef DZ_Expansion_Navigation
modded class ExpansionMapMenu
{
	protected ref ZenCircleMapMarkerRenderer m_ZenCircleMapMarkerRenderer;

	override Widget Init()
	{
		Widget root = super.Init();

		if (root && !m_ZenCircleMapMarkerRenderer && GetMap())
			m_ZenCircleMapMarkerRenderer = new ZenCircleMapMarkerRenderer(GetMap(), root);

		ZenCircleMapMarkers_KeepExpansionUIAboveCircles(root);
		return root;
	}

	override void OnShow()
	{
		super.OnShow();

		if (m_ZenCircleMapMarkerRenderer)
			m_ZenCircleMapMarkerRenderer.Show();

		ZenCircleMapMarkers_KeepExpansionUIAboveCircles(GetLayoutRoot());
	}

	override void OnHide()
	{
		if (m_ZenCircleMapMarkerRenderer)
			m_ZenCircleMapMarkerRenderer.Hide();

		super.OnHide();
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		if (m_ZenCircleMapMarkerRenderer)
			m_ZenCircleMapMarkerRenderer.Update(timeslice);

		ZenCircleMapMarkers_KeepExpansionUIAboveCircles(GetLayoutRoot());
	}

	protected void ZenCircleMapMarkers_KeepExpansionUIAboveCircles(Widget root)
	{
		if (m_Markers)
		{
			foreach (ExpansionMapWidgetBase expansionMapMarker : m_Markers)
			{
				if (!expansionMapMarker)
					continue;

				Widget expansionMarkerRoot = expansionMapMarker.GetLayoutRoot();
				if (expansionMarkerRoot && expansionMarkerRoot.GetSort() < ZenCircleMapMarkerConstants.MARKER_SORT)
					expansionMarkerRoot.SetSort(ZenCircleMapMarkerConstants.MARKER_SORT);
			}
		}

		if (!root)
			return;

		Widget expansionToolsContainer = root.FindAnyWidget("Map_Tools_Container");
		if (expansionToolsContainer && expansionToolsContainer.GetSort() < ZenCircleMapMarkerConstants.TOOLS_SORT)
			expansionToolsContainer.SetSort(ZenCircleMapMarkerConstants.TOOLS_SORT);

		Widget zenMapFogToolsOverlay = root.FindAnyWidget("ZMF_Map_Tools_Container");
		if (zenMapFogToolsOverlay && zenMapFogToolsOverlay.GetSort() < ZenCircleMapMarkerConstants.TOOLS_SORT)
			zenMapFogToolsOverlay.SetSort(ZenCircleMapMarkerConstants.TOOLS_SORT);
	}
};
#endif
