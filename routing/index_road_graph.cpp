#include "routing/index_road_graph.hpp"

#include "routing/routing_exceptions.hpp"
#include "routing/transit_graph.hpp"

#include <cstdint>

using namespace std;

namespace routing
{
IndexRoadGraph::IndexRoadGraph(shared_ptr<NumMwmIds> numMwmIds, IndexGraphStarter & starter,
                               vector<Segment> const & segments, vector<Junction> const & junctions,
                               Index & index)
  : m_index(index), m_numMwmIds(numMwmIds), m_starter(starter), m_segments(segments)
{
  //    j0     j1     j2     j3
  //    *--s0--*--s1--*--s2--*
  CHECK_EQUAL(segments.size() + 1, junctions.size(), ());

  for (size_t i = 0; i < junctions.size(); ++i)
  {
    Junction const & junction = junctions[i];
    if (i > 0)
      m_endToSegment[junction].push_back(segments[i - 1]);
    if (i < segments.size())
      m_beginToSegment[junction].push_back(segments[i]);
  }
}

void IndexRoadGraph::GetOutgoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  GetEdges(junction, true, edges);
}

void IndexRoadGraph::GetIngoingEdges(Junction const & junction, TEdgeVector & edges) const
{
  GetEdges(junction, false, edges);
}

double IndexRoadGraph::GetMaxSpeedKMPH() const
{
  // Value doesn't matter.
  // It is used in CalculateMaxSpeedTimes only.
  // Then SingleMwmRouter::RedressRoute overwrites time values.
  //
  // TODO: remove this stub after transfering Bicycle and Pedestrian to index routing.
  return 0.0;
}

void IndexRoadGraph::GetEdgeTypes(Edge const & edge, feature::TypesHolder & types) const
{
  if (edge.IsFake())
  {
    types = feature::TypesHolder(feature::GEOM_LINE);
    return;
  }

  FeatureID const featureId = edge.GetFeatureId();
  FeatureType ft;
  Index::FeaturesLoaderGuard loader(m_index, featureId.m_mwmId);
  if (!loader.GetFeatureByIndex(featureId.m_index, ft))
  {
    LOG(LERROR, ("Can't load types for feature", featureId));
    return;
  }

  ASSERT_EQUAL(ft.GetFeatureType(), feature::GEOM_LINE, ());
  types = feature::TypesHolder(ft);
}

void IndexRoadGraph::GetJunctionTypes(Junction const & junction, feature::TypesHolder & types) const
{
  // TODO: implement method to support PedestrianDirection::LiftGate, PedestrianDirection::Gate
  types = feature::TypesHolder();
}

bool IndexRoadGraph::IsRouteEdgesImplemented() const
{
  return true;
}

bool IndexRoadGraph::IsRouteSegmentsImplemented() const { return true; }

void IndexRoadGraph::GetRouteEdges(TEdgeVector & edges) const
{
  edges.clear();
  edges.reserve(m_segments.size());

  for (Segment const & segment : m_segments)
  {
    auto featureId = FeatureID();

    if (!IndexGraphStarter::IsFakeSegment(segment) && !TransitGraph::IsTransitSegment(segment))
    {
      platform::CountryFile const & file = m_numMwmIds->GetFile(segment.GetMwmId());
      MwmSet::MwmId const mwmId = m_index.GetMwmIdByCountryFile(file);
      featureId = FeatureID(mwmId, segment.GetFeatureId());
    }
    edges.emplace_back(featureId, segment.IsForward(), segment.GetSegmentIdx(),
                       m_starter.GetJunction(segment, false /* front */),
                       m_starter.GetJunction(segment, true /* front */));
  }
}

void IndexRoadGraph::GetRouteSegments(std::vector<Segment> & segments) const
{
  segments = m_segments;
}

void IndexRoadGraph::GetEdges(Junction const & junction, bool isOutgoing, TEdgeVector & edges) const
{
  edges.clear();

  vector<SegmentEdge> segmentEdges;
  vector<SegmentEdge> tmpEdges;
  for (Segment const & segment : GetSegments(junction, isOutgoing))
  {
    tmpEdges.clear();
    m_starter.GetEdgesList(segment, isOutgoing, tmpEdges);
    segmentEdges.insert(segmentEdges.end(), tmpEdges.begin(), tmpEdges.end());
  }

  for (SegmentEdge const & segmentEdge : segmentEdges)
  {
    Segment const & segment = segmentEdge.GetTarget();
    if (IndexGraphStarter::IsFakeSegment(segment))
      continue;

    platform::CountryFile const & file = m_numMwmIds->GetFile(segment.GetMwmId());
    MwmSet::MwmId const mwmId = m_index.GetMwmIdByCountryFile(file);

    edges.emplace_back(FeatureID(mwmId, segment.GetFeatureId()), segment.IsForward(),
                       segment.GetSegmentIdx(), m_starter.GetJunction(segment, false /* front */),
                       m_starter.GetJunction(segment, true /* front */));
  }
}

vector<Segment> const & IndexRoadGraph::GetSegments(Junction const & junction,
                                                    bool isOutgoing) const
{
  auto const & junctionToSegment = isOutgoing ? m_endToSegment : m_beginToSegment;

  auto const it = junctionToSegment.find(junction);
  CHECK(it != junctionToSegment.cend(),
        ("junctionToSegment doesn't contain", junction, ", isOutgoing =", isOutgoing));
  return it->second;
}
}  // namespace routing
