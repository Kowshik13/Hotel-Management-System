#include "ui/Router.h"
#include "ui/AppContext.h"

int main(){
    hms::UserRepository        users;
    hms::RoomsRepository       rooms;
    hms::HotelRepository       hotels;
    hms::BookingRepository     bookings;
    hms::RestaurantRepository  restaurants;

    hms::AppContext ctx{
            .svc = { &users, &rooms, &hotels, &bookings, &restaurants },
            .currentUser = std::nullopt,
            .running = true
    };

    hms::ui::Run(ctx);
    return 0;
}
