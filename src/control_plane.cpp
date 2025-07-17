#include <atomic>
#include <control_plane.h>
#include <iostream>

namespace {
    std::atomic<uint32_t> g_teid_generator{1000};
    std::atomic<uint32_t> g_ip_generator{1};
} // namespace

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_cp_teid(uint32_t cp_teid) const {
    auto it = _pdns.find(cp_teid);
    return (it != _pdns.end()) ? it->second : nullptr;
}

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_ip_address(const boost::asio::ip::address_v4 &ip) const {
    auto it = _pdns_by_ue_ip_addr.find(ip);
    return (it != _pdns_by_ue_ip_addr.end()) ? it->second : nullptr;
}

std::shared_ptr<bearer> control_plane::find_bearer_by_dp_teid(uint32_t dp_teid) const {
    auto it = _bearers.find(dp_teid);
    return (it != _bearers.end()) ? it->second : nullptr;
}

void control_plane::add_apn(std::string apn_name, boost::asio::ip::address_v4 apn_gateway) {
    _apns[std::move(apn_name)] = std::move(apn_gateway);
}

std::shared_ptr<pdn_connection> control_plane::create_pdn_connection(const std::string &apn,
                                                                     boost::asio::ip::address_v4 sgw_addr,
                                                                     uint32_t sgw_cp_teid) {
    auto apn_it = _apns.find(apn);
    if (apn_it == _apns.end()) {
        throw std::runtime_error("APN not found: " + apn);
    }

    auto ue_ip = boost::asio::ip::address_v4(g_ip_generator++);
    auto cp_teid = g_teid_generator++;

    auto pdn = pdn_connection::create(cp_teid, apn_it->second, ue_ip);
    pdn->set_sgw_cp_teid(sgw_cp_teid);
    pdn->set_sgw_addr(sgw_addr);

    _pdns[cp_teid] = pdn;
    _pdns_by_ue_ip_addr[ue_ip] = pdn;

    return pdn;
}

std::shared_ptr<bearer> control_plane::create_bearer(const std::shared_ptr<pdn_connection> &pdn, uint32_t sgw_teid) {
    auto dp_teid = g_teid_generator++;
    auto b = std::make_shared<bearer>(dp_teid, *pdn);
    b->set_sgw_dp_teid(sgw_teid);
    pdn->add_bearer(b);

    _bearers[dp_teid] = b;
    return b;
}

void control_plane::delete_pdn_connection(uint32_t cp_teid) {
    auto it = _pdns.find(cp_teid);
    if (it != _pdns.end()) {
        auto pdn = it->second;
        _pdns_by_ue_ip_addr.erase(pdn->get_ue_ip_addr());
        _pdns.erase(it);
    }
}

void control_plane::delete_bearer(uint32_t dp_teid) {
    auto it = _bearers.find(dp_teid);
    if (it != _bearers.end()) {
        auto pdn = it->second->get_pdn_connection();
        if (pdn) {
            pdn->remove_bearer(dp_teid);
        }
        _bearers.erase(it);
    }
}
