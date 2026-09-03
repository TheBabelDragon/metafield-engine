#pragma once
#include "engine/substrate/channel.hpp"
#include <cstdint>
namespace mf {
enum class Quantity : std::uint8_t {
    Unknown = 0,
    Temperature, Pressure, MassDensity, EnergyDensity, ChargeDensity,
    ElectricPotential, MagneticFlux, Radiance, SoundPressure,
    Humidity, Concentration, VelocityX, VelocityY, VelocityZ,
    Acceleration, Information, COUNT
};
inline const char* quantity_name(Quantity q) {
    switch (q) {
        case Quantity::Temperature: return "temperature";
        case Quantity::Pressure: return "pressure";
        case Quantity::MassDensity: return "mass_density";
        case Quantity::EnergyDensity: return "energy_density";
        case Quantity::ChargeDensity: return "charge_density";
        case Quantity::ElectricPotential: return "electric_potential";
        case Quantity::MagneticFlux: return "magnetic_flux";
        case Quantity::Radiance: return "radiance";
        case Quantity::SoundPressure: return "sound_pressure";
        case Quantity::Humidity: return "humidity";
        case Quantity::Concentration: return "concentration";
        case Quantity::VelocityX: return "velocity_x";
        case Quantity::VelocityY: return "velocity_y";
        case Quantity::VelocityZ: return "velocity_z";
        case Quantity::Acceleration: return "acceleration";
        case Quantity::Information: return "information";
        default: return "unknown";
    }
}
inline const char* quantity_unit(Quantity q) {
    switch (q) {
        case Quantity::Temperature: return "K";
        case Quantity::Pressure: return "Pa";
        case Quantity::MassDensity: return "kg/m3";
        case Quantity::EnergyDensity: return "J/m3";
        case Quantity::ChargeDensity: return "C/m3";
        case Quantity::ElectricPotential: return "V";
        case Quantity::MagneticFlux: return "T";
        case Quantity::Radiance: return "W/sr/m2";
        case Quantity::SoundPressure: return "Pa";
        case Quantity::Humidity: return "1";
        case Quantity::Concentration: return "mol/m3";
        case Quantity::VelocityX:
        case Quantity::VelocityY:
        case Quantity::VelocityZ: return "m/s";
        case Quantity::Acceleration: return "m/s2";
        case Quantity::Information: return "1";
        default: return "";
    }
}
inline Channel channel_for(Quantity q) {
    switch (q) {
        case Quantity::MassDensity: return Channel::Matter;
        case Quantity::EnergyDensity: return Channel::Energy;
        case Quantity::Temperature: return Channel::Temperature;
        case Quantity::Pressure: return Channel::Pressure;
        case Quantity::ChargeDensity: return Channel::Charge;
        case Quantity::VelocityX: return Channel::MomentumX;
        case Quantity::VelocityY: return Channel::MomentumY;
        case Quantity::VelocityZ: return Channel::MomentumZ;
        case Quantity::Information: return Channel::Information;
        default: return Channel::Information;
    }
}
} // namespace mf
