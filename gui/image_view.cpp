#include "image_view.hpp"
#include "controller.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/display_list.hpp"

#include "../base/matrix.hpp"

#include "../geometry/transformations.hpp"

namespace gui
{
  ImageView::Params::Params()
  {}

  ImageView::ImageView(Params const & p)
    : base_t(p),
      m_boundRects(1)
  {
    m_image = p.m_image;
  }

  void ImageView::cache()
  {
    graphics::Screen * cs = m_controller->GetCacheScreen();

    m_displayList.reset();
    m_displayList.reset(cs->createDisplayList());

    cs->beginFrame();
    cs->setDisplayList(m_displayList.get());

    math::Matrix<double, 3, 3> m =
        math::Shift(
          math::Identity<double, 3>(),
          -(int)m_image.m_size.x / 2, -(int)m_image.m_size.y / 2);

    uint32_t imageResID = cs->mapInfo(m_image);
    cs->drawImage(m, imageResID, depth());

    cs->setDisplayList(0);
    cs->endFrame();
  }

  void ImageView::purge()
  {
    m_displayList.reset();
  }

  vector<m2::AnyRectD> const & ImageView::boundRects() const
  {
    if (isDirtyRect())
    {
      m2::PointD sz(m_image.m_size);

      m2::PointD pt = computeTopLeft(sz,
                                     pivot(),
                                     position());


      m_boundRects[0] = m2::AnyRectD(m2::RectD(pt, pt + sz));

      setIsDirtyRect(false);
    }

    return m_boundRects;
  }

  void ImageView::draw(graphics::OverlayRenderer * r,
                       math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyLayout();

      m2::PointD pt = computeTopLeft(m_image.m_size,
                                     pivot() * m,
                                     position());

      math::Matrix<double, 3, 3> drawM = math::Shift(math::Identity<double, 3>(),
                                                     pt.x + m_image.m_size.x / 2,
                                                     pt.y + m_image.m_size.y / 2);

      r->drawDisplayList(m_displayList.get(), drawM * m);
    }
  }
}
