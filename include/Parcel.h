#ifndef PARCEL_H
#define PARCEL_H

#include <string>
#include <vector>

// Delivery status enum and constants
namespace ParcelStatus {
    const std::string REGISTERED = "Registered";
    const std::string DISPATCHED = "Dispatched";
    const std::string IN_TRANSIT = "In Transit";
    const std::string DELIVERED  = "Delivered";
    const std::string CANCELLED  = "Cancelled";

    inline bool isValidStatus(const std::string& status) {
        return (status == REGISTERED || status == DISPATCHED ||
                status == IN_TRANSIT  || status == DELIVERED  ||
                status == CANCELLED);
    }
}

struct Parcel {
    std::string trackingID;
    std::string sender;
    std::string recipient;
    std::string destination;
    std::string status;          // "Registered", "Dispatched", "In Transit", "Delivered", "Cancelled"
    std::string registrationDate;// YYYY-MM-DD
    std::string dispatchDate;    // YYYY-MM-DD or "-"

    Parcel()
        : trackingID(""), sender(""), recipient(""), destination(""),
          status(ParcelStatus::REGISTERED), registrationDate(""), dispatchDate("-") {}

    Parcel(std::string id, std::string snd, std::string rcv, std::string dest,
           std::string stat, std::string regDate, std::string dispDate = "-")
        : trackingID(std::move(id)), sender(std::move(snd)), recipient(std::move(rcv)),
          destination(std::move(dest)), status(std::move(stat)),
          registrationDate(std::move(regDate)), dispatchDate(std::move(dispDate)) {
        if (dispatchDate.empty()) dispatchDate = "-";
    }

    // Serialize to pipe-separated format: trackingID | sender | recipient | destination | status | registrationDate | dispatchDate
    std::string toFileLine() const {
        return trackingID + " | " + sender + " | " + recipient + " | " +
               destination + " | " + status + " | " + registrationDate + " | " +
               (dispatchDate.empty() ? "-" : dispatchDate);
    }
};

#endif // PARCEL_H

