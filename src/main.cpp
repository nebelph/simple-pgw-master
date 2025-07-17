#include <data_plane.h>
#include <iostream>

class simple_data_plane : public data_plane {
public:
    explicit simple_data_plane(control_plane &cp) : data_plane(cp) {}

protected:
    void forward_packet_to_sgw(boost::asio::ip::address_v4 sgw_addr, uint32_t sgw_dp_teid, Packet &&packet) override {
        std::cout << "Forwarded to SGW (" << sgw_addr.to_string() << ") teid=" << sgw_dp_teid
                  << ", size=" << packet.size() << '\n';
    }

    void forward_packet_to_apn(boost::asio::ip::address_v4 apn_gateway, Packet &&packet) override {
        std::cout << "Forwarded to APN (" << apn_gateway.to_string() << "), size=" << packet.size() << '\n';
    }
};

int main() {
    control_plane cp;

    std::string apn = "internet";
    auto apn_gw = boost::asio::ip::make_address_v4("10.0.0.1");
    auto sgw_addr = boost::asio::ip::make_address_v4("10.0.0.2");
    uint32_t sgw_cp_teid = 100;

    cp.add_apn(apn, apn_gw);

    auto pdn = cp.create_pdn_connection(apn, sgw_addr, sgw_cp_teid);
    auto bearer = cp.create_bearer(pdn, 200);
    pdn->set_default_bearer(bearer);

    simple_data_plane dp(cp);

    // Сымитируем пакеты
    dp.handle_uplink(bearer->get_dp_teid(), {1, 2, 3});
    dp.handle_downlink(pdn->get_ue_ip_addr(), {4, 5, 6});

    return 0;
}
