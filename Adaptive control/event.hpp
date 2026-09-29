#include <string>
#include <chrono>

#include <boost/json.hpp>
#include <boost/uuid/detail/md5.hpp> //For MD5 hashing
#include <boost/algorithm/hex.hpp> //Also for hashing
#include <boost/date_time/posix_time/posix_time.hpp> //For ISO time 

#pragma once


class event {
public:
    std::string id, name, value, timestamp, createdTimestamp, workstationComponent, workstationId;

    event() {}; 

    event(std::string n, std::string v, std::string t, std::string c) {
        workstationId = "HAAS-VF2";
        name = n;
        value = v;
        timestamp = t;
        workstationComponent = c;
        id = MD5hash(workstationId + workstationComponent + name + value + timestamp);
    }

    event(const event& c) {
        workstationId = c.workstationId;
        id = c.id;
        name = c.name;
        value = c.value;
        timestamp = c.timestamp;
        workstationComponent = c.workstationComponent;
    }

    boost::json::object makeJSON() {
        boost::json::object obj;
        obj.emplace("id", id); //md5(workstationId + workstationComponent + name + value + timestamp)
        obj.emplace("name", name); //Name of the event, per MTC specs
        obj.emplace("value", value); //Value of the event, per MTC specs
        obj.emplace("timestamp", timestamp);
        obj.emplace("createdTimestamp", 
            boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::universal_time()) + "+00:00"); //ISO date
        obj.emplace("workstationCode", "Haas-VF2");
        obj.emplace("workstationId", workstationId); //16 digit MD5 hash, fix this 
        obj.emplace("workstationComponent", workstationComponent);
        obj.emplace("nameId", MD5hash(workstationId + workstationComponent + name)); //Unique name, replicate id 
        obj.emplace("rawName", value); //Same as value? Different in gcode.program, gcode.mcode, etc.

        return obj;
    }

    std::string MD5hash(std::string s) {
        boost::uuids::detail::md5 hash;
        boost::uuids::detail::md5::digest_type digest;
        hash.process_bytes(s.data(), s.size());
        hash.get_digest(digest);
        const auto intDigest = reinterpret_cast<const int*>(&digest);
        std::string result;
        boost::algorithm::hex(intDigest,
            intDigest + (sizeof(boost::uuids::detail::md5::digest_type) / sizeof(int)),
            std::back_inserter(result));
        return result;
    }

    friend bool operator==(const event& obj, const event& obj2) {
        return (obj.value == obj2.value);
    }

    friend bool operator==(const event& obj, std::string s) {
        return (obj.value == s);
    }

    friend bool operator!=(const event& obj, const event& obj2) {
        return (obj.value != obj2.value);
    }

    friend bool operator!=(const event& obj, std::string s) {
        return !(obj.value == s);
    }
};