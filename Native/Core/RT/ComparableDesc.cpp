/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ComparableDesc.h"
#include "ComparableImpl.h"

namespace Fabric
{
  namespace RT
  {
    ComparableDesc::ComparableDesc(
      std::string const &userNameBase,
      std::string const &userNameArraySuffix,
      RC::ConstHandle<ComparableImpl> const &comparableImpl
      )
      : Desc(
        userNameBase,
        userNameArraySuffix,
        comparableImpl
        )
      , m_comparableImpl( comparableImpl )
    {
    }

    RC::ConstHandle<RT::ComparableImpl> ComparableDesc::getImpl() const
    {
      return m_comparableImpl;
    }
  }
}
