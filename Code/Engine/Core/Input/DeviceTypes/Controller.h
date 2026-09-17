#pragma once

#include <Core/Input/InputDevice.h>

struct WPhysicalControllerInput
{
  using StorageType = WUInt32;

  enum Enum
  {
    Default = 0,

    Start = W_BIT(0),          ///< The 'Start' button
    Back = W_BIT(1),           ///< The 'Back' button
    FrontButton = W_BIT(2),    ///< Any button on the front/top of the controller
    ShoulderButton = W_BIT(3), ///< Any shoulder button

    AnyInput = 0xFFFFFFFF,      ///< Any of the available input categories above
  };

  struct Bits
  {
    StorageType Start : 1;
    StorageType Back : 1;
    StorageType FrontButton : 1;
    StorageType ShoulderButton : 1;
  };
};

/// This class is the base class for all controller type input devices.
///
/// This class is derived from WInputDevice but adds some interface functions common to most controllers.
/// This class adds functions to query and modify the state about controller vibration, about the mapping of
/// physical controllers to virtual ones (which controller index triggers which controller input slots) and
/// also allows to query which controller is actually connected.
class W_CORE_DLL WInputDeviceController : public WInputDevice
{
  W_ADD_DYNAMIC_REFLECTION(WInputDeviceController, WInputDevice);

public:
  enum
  {
    MaxControllers = 4,
    VibrationSamplesPerSecond = 16,
    VibrationTrackSeconds = 2,
    MaxVibrationSamples = VibrationSamplesPerSecond * VibrationTrackSeconds, // With constant power-of-two samples some code should get more efficient
  };

  /// Describes which vibration motor to configure.
  struct Motor
  {
    enum Enum
    {
      LeftMotor,
      RightMotor,
      ENUM_COUNT
    };
  };

  WInputDeviceController();

  /// Enables or disables vibration on the given controller (virtual index).
  /// If it is disabled, the controller will never vibrate, even if vibration profiles are sent to it.
  void EnableVibration(WUInt8 uiVirtual, bool bEnable);

  /// Checks whether vibration is enabled on the given controller (virtual index).
  bool IsVibrationEnabled(WUInt8 uiVirtual) const;

  /// Sets the vibration strength for the given controller and motor. \a fValue is a value between 0 and 1.
  ///
  /// From now on the controller will be vibrating (unless vibration is disabled), until the value is reset to zero.
  /// This kind of vibration is always combined with vibration tracks (the maximum of both values is applied at any
  /// one time). Using this function is it possible to have more direct control over vibration, while the
  /// vibration tracks are convenient for the most common (short) effects.
  void SetVibrationStrength(WUInt8 uiVirtual, Motor::Enum motor, float fValue);

  /// Returns the amount of (constant) vibration that is currently set on this controller.
  float GetVibrationStrength(WUInt8 uiVirtual, Motor::Enum motor);

  /// Sets to which virtual controller a physical controller pushes its input.
  ///
  /// If iVirtualController is negative, the given physical controller is not used.
  /// Multiple physical controllers may push their input to the same virtual controller,
  /// in which case multiple people can control the same thing.
  ///
  /// By default all physical controllers push their input to virtual controller 0.
  /// So any controller can be used to play the game.
  /// If that is not desired, change the mapping at startup.
  ///
  /// You can use this feature to let the player pick up any controller, detect which one it is (e.g. by forcing them to press 'Start')
  /// and then map that physical controller index to the virtual index 0 (ie. player 1).
  /// See also GetRecentPhysicalControllerInput() to detect controller usage.
  void SetPhysicalControllerMapping(WUInt8 uiPhysicalController, WInt8 iVirtualController);

  /// Returns to which virtual controller the given physical controller pushes its input.
  ///
  /// If negative, that means the physical controller is not used.
  /// Multiple physical controllers may map to the same virtual controller, which would allow two people to control the same object.
  WInt8 GetPhysicalControllerMapping(WUInt8 uiPhysical) const;

  /// Queries whether the controller with the given physical index is connected to the computer.
  /// This may change at any time.
  virtual bool IsPhysicalControllerConnected(WUInt8 uiPhysical) const = 0;

  /// Adds a short 'vibration track' (a sequence of vibrations) to the given controller.
  ///
  /// Each controller has a short (typically 2 second) buffer for vibration values, that it will play.
  /// This allows to have different 'tracks' for different events, which are simply set on the controller.
  /// You can add an unlimited amount of tracks on a controller, the controller stores the maximum of all tracks
  /// and plays that.
  /// That means whenever the player shoots, or is hit etc., you can add a vibration track to the controller
  /// and it will be combined with all other tracks and played (no memory allocations are required).
  ///
  /// \param uiVirtual The virtual index of the controller.
  /// \param eMotor Which motor to apply the track on.
  /// \param fVibrationTrackValue An array of at least \a uiSamples float values, each between 0 and 1.
  /// \param uiSamples How many samples \a fVibrationTrackValue contains. A maximum of MaxVibrationSamples samples is used.
  /// \param fScalingFactor Additional scaling factor to apply to all values in \a fVibrationTrackValue.
  void AddVibrationTrack(WUInt8 uiVirtual, Motor::Enum motor, float* pVibrationTrackValue, WUInt32 uiSamples, float fScalingFactor = 1.0f);

  /// Returns a bitmask that specifies what kind of input a controller recently (last frame) had.
  ///
  /// Use this to identify which controller a user has picked up and wants to use.
  /// This is not meant to be used for handling input, only to know which physical controller to map to which virtual controller.
  WBitflags<WPhysicalControllerInput> GetRecentPhysicalControllerInput(WUInt8 uiPhysical) const;

protected:
  /// Combines the constant vibration and vibration tracks and applies them on each controller.
  ///
  /// This function needs to be called by a derived implementation in its UpdateInputSlotValues() function.
  /// It will call ApplyVibration() for each controller and motor with the current value. It already takes care
  /// of whether vibration is enabled or disabled, and also mapping virtual to physical controllers.
  void UpdateVibration(WTime tTimeDifference);

  /// To be filled out by derived implementations.
  /// Should set the proper bits every frame when there was such user input.
  /// Can be used by games to detect whether a player wants to use this physical controller, and potentially remap it.
  /// Not meant for actually handling input.
  WBitflags<WPhysicalControllerInput> m_RecentPhysicalControllerInput[MaxControllers];

private:
  /// Must be implemented by a derived controller implementation. Should set apply the vibration for the given physical controller
  /// and motor with the given strength.
  ///
  /// A strength value of zero will be passed in whenever no vibration is required. No extra resetting needs to be implemented.
  virtual void ApplyVibration(WUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) = 0;

  WUInt32 m_uiVibrationTrackPos = 0;
  float m_fVibrationTracks[MaxControllers][Motor::ENUM_COUNT][MaxVibrationSamples];
  bool m_bVibrationEnabled[MaxControllers];
  WInt8 m_iPhysicalToVirtualControllerMapping[MaxControllers]; // maps from physical device index to virtual controller index
  float m_fVibrationStrength[MaxControllers][Motor::ENUM_COUNT];
};
