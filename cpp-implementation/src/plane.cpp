
module plane;

namespace cg
{
std::optional<hitevent> get_ray_plane_hit(const plane& p, ray r, interval i)
{
  // ray points are every p where p = start + t*dir for some t [1]
  // plane points are every p where (plane_position - p) dot plane_normal = 0
  // [2]
  //
  // we combine this two equations to get the points they share
  // replace p in [2] for
  // (plane_position -(start+t*dir)) dot plane_normal = 0 [3]
  // we use distribution property of the dot product
  // normal dot pos - normal dot start - normal dot (t*dir) = 0
  // reorganizing for t
  // normal dot pos - normal dot start - normal*t dot dir = 0
  // normal dot pos - normal dot start = t* (normal dot dir)
  // normal dot (pos-start)/normal dot dir = t
  // we are going to remove the solutions where t approaches zero
  // with an epsilon of 1e-6
  const interval almost_zero{-1e-6, 1e-6};
  const auto denominator = glm::dot(p.normal, r.dir);
  if (almost_zero.contains(denominator))
    return std::nullopt;

  const auto t = glm::dot(p.normal, (p.point - r.pos)) / denominator;
  if (!i.contains(t) or t < 0)
  {
    return std::nullopt;
  }

  const bool front_face = denominator < 0.0f;
  const auto normal = front_face ? p.normal : -p.normal;
  const hitevent ev{
      t,
      r.at(t),
      normal,
      p.material_id,
  };
  return ev;
}

} // namespace cg
