#ifndef EUI48_ADDRESS_H
#define EUI48_ADDRESS_H

#include <stdint.h>
#include <ostream>

namespace ns3 {

class Address;

/**
 * \brief an EUI-48 address
 *
 * This class can contain 48 bit IEEE addresses.
 */
class Eui48Address
{
public:
  Eui48Address ();
  /**
   * \param str a string representing the new Eui48Address
   *
   * The format of the string is "xx:xx:xx:xx:xx:xx"
   */
  Eui48Address (const char *str);

  /**
   * \param buffer address in network order
   *
   * Copy the input address to our internal buffer.
   */
  void CopyFrom (const uint8_t buffer[6]);
  /**
   * \param buffer address in network order
   *
   * Copy the internal address to the input buffer.
   */
  void CopyTo (uint8_t buffer[6]) const;

  /**
   * \returns a new Address instance
   *
   * Convert an instance of this class to a polymorphic Address instance.
   */
  operator Address ();
  /**
   * \param address a polymorphic address
   * \returns a new Eui48Address from the polymorphic address
   * 
   * This function performs a type check and asserts if the
   * type of the input address is not compatible with an
   * Eui48Address.
   */
  static Eui48Address ConvertFrom (const Address &address);
  /**
   * \returns true if the address matches, false otherwise.
   */
  static bool IsMatchingType (const Address &address);
  /**
   * Allocate a new Eui48Address.
   */
  static Eui48Address Allocate (void);
private:
  /**
   * \returns a new Address instance
   *
   * Convert an instance of this class to a polymorphic Address instance.
   */
  Address ConvertTo (void) const;
  static uint8_t GetType (void);
  uint8_t m_address[6];
};

bool operator == (const Eui48Address &a, const Eui48Address &b);
bool operator != (const Eui48Address &a, const Eui48Address &b);
std::ostream& operator<< (std::ostream& os, const Eui48Address & address);

} // namespace ns3

#endif /* EUI48_ADDRESS_H */
