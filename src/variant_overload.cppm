export module variant_overload;
export namespace cg
{
template <class... Ts> struct overload : Ts...
{
    using Ts::operator()...;
};

} // namespace cg
