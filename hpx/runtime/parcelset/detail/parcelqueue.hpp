//  Copyright (c) 2014-2017 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_DETAIL_PARCELQUEUE_HPP
#define HPX_PARCELSET_DETAIL_PARCELQUEUE_HPP

#include <hpx/runtime/parcelset_fwd.hpp>
#include <hpx/runtime/parcelset/locality.hpp>
#include <hpx/runtime/parcelset/parcel.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/tuple.hpp>

#include <map>
#include <vector>
#include <set>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace parcelset { namespace detail
{
    class parcelqueue
    {
        typedef util::tuple<
            std::vector<parcel>
          , std::vector<write_handler_type>
        > map_second_type;
        typedef std::map<locality, map_second_type> pending_parcels_map;
        typedef std::set<locality> pending_parcels_destinations;
    public:

        void enqueue_parcel(locality const& locality_id,
            parcel&& p, write_handler_type&& f)
        {
            typedef pending_parcels_map::mapped_type mapped_type;

            mapped_type& e = pending_parcels_[locality_id];
            util::get<0>(e).push_back(std::move(p));
            util::get<1>(e).push_back(std::move(f));

            parcel_destinations_.insert(locality_id);
        }

        void enqueue_parcels(locality const& locality_id,
            std::vector<parcel>&& parcels,
            std::vector<write_handler_type>&& handlers)
        {
            typedef pending_parcels_map::mapped_type mapped_type;
            HPX_ASSERT(parcels.size() == handlers.size());

            mapped_type& e = pending_parcels_[locality_id];
            if (util::get<0>(e).empty())
            {
                HPX_ASSERT(util::get<1>(e).empty());
                std::swap(util::get<0>(e), parcels);
                std::swap(util::get<1>(e), handlers);
            }
            else
            {
                HPX_ASSERT(util::get<0>(e).size() == util::get<1>(e).size());
                std::size_t new_size = util::get<0>(e).size() + parcels.size();
                util::get<0>(e).reserve(new_size);

                std::move(parcels.begin(), parcels.end(),
                    std::back_inserter(util::get<0>(e)));
                util::get<1>(e).reserve(new_size);
                std::move(handlers.begin(), handlers.end(),
                    std::back_inserter(util::get<1>(e)));
            }

            parcel_destinations_.insert(locality_id);
        }

        bool dequeue_parcels(locality const& locality_id,
            std::vector<parcel>& parcels,
            std::vector<write_handler_type>& handlers)
        {
            typedef pending_parcels_map::iterator iterator;

            iterator it = pending_parcels_.find(locality_id);

            // do nothing if parcels have already been picked up by
            // another thread
            if (it != pending_parcels_.end() && !util::get<0>(it->second).empty())
            {
                HPX_ASSERT(it->first == locality_id);
                HPX_ASSERT(handlers.size() == 0);
                HPX_ASSERT(handlers.size() == parcels.size());
                HPX_ASSERT(util::get<0>(it->second).size() == util::get<1>(it->second).size());
                std::swap(parcels, util::get<0>(it->second));
                HPX_ASSERT(util::get<0>(it->second).size() == 0);
                std::swap(handlers, util::get<1>(it->second));
                HPX_ASSERT(handlers.size() == parcels.size());

                if (handlers.empty()) return false;
//                 HPX_ASSERT(!handlers.empty());
            }
            else
            {
                HPX_ASSERT(util::get<1>(it->second).empty());
                return false;
            }

            parcel_destinations_.erase(locality_id);

            return true;
        }

        bool dequeue_parcel(locality& dest, parcel& p, write_handler_type& handler)
        {
            for (auto &pending: pending_parcels_)
            {
                auto &parcels = util::get<0>(pending.second);
                if (!parcels.empty())
                {
                    auto& handlers = util::get<1>(pending.second);
                    dest = pending.first;
                    p = std::move(parcels.back());
                    parcels.pop_back();
                    handler = std::move(handlers.back());
                    handlers.pop_back();

                    if (parcels.empty())
                    {
                        pending_parcels_.erase(dest);
                    }
                    return true;
                }
            }
            return false;
        }

        std::vector<locality> destinations() const
        {
            std::vector<locality> res;

            res.reserve(parcel_destinations_.size());
            for (locality const& loc : parcel_destinations_)
            {
                res.push_back(loc);
            }

            return res;
        }

        bool has_destination(locality const& locality_id) const
        {
            auto it = pending_parcels_.find(locality_id);
            if (it == pending_parcels_.end() || util::get<0>(it->second).empty())
                return false;

            return true;
        }

        std::size_t size() const
        {
            std::int64_t count = 0;
            for (auto && p : pending_parcels_)
            {
                count += hpx::util::get<0>(p.second).size();
                HPX_ASSERT(
                    hpx::util::get<0>(p.second).size() ==
                    hpx::util::get<1>(p.second).size());
            }
            return count;
        }

    private:
        pending_parcels_map pending_parcels_;

        pending_parcels_destinations parcel_destinations_;
    };
}}}

#endif
