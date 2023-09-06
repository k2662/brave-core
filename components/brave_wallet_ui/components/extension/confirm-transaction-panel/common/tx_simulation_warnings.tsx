// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'
import Alert from '@brave/leo/react/alert'

// types
import { BraveWallet } from '../../../../constants/types'

// styles
import { FullWidth } from '../../../shared/style'
import { WarningsList } from '../style'
import {
  CriticalWarningIcon,
  WarningCollapse,
  WarningTriangleFilledIcon
} from '../confirm_simulated_tx_panel.styles'

export function SimulationWarnings({
  hasCriticalWarnings,
  isWarningCollapsed,
  setIsWarningCollapsed,
  txSimulation
}: {
  txSimulation:
    | BraveWallet.EVMSimulationResponse
    | BraveWallet.SolanaSimulationResponse
  isWarningCollapsed: boolean
  hasCriticalWarnings: boolean
  setIsWarningCollapsed: React.Dispatch<React.SetStateAction<boolean>>
}): JSX.Element | null {
  // render
  return txSimulation.warnings.length > 0 ? (
    <FullWidth>
      {txSimulation.warnings.length > 1 ? (
        <WarningCollapse
          isOpen={!isWarningCollapsed}
          isCritical={hasCriticalWarnings}
          onToggle={() => setIsWarningCollapsed((prev) => !prev)}
          title={
            // TODO: locale
            hasCriticalWarnings
              ? 'Unsafe action with risk of loss'
              : `We found ${txSimulation.warnings.length} issue(s).`
          }
        >
          <div slot='icon'>
            {hasCriticalWarnings ? (
              <CriticalWarningIcon />
            ) : (
              <WarningTriangleFilledIcon />
            )}
          </div>

          <WarningsList>
            {txSimulation.warnings.map((warning) => (
              <li key={warning.message}>{warning.message}</li>
            ))}
          </WarningsList>
        </WarningCollapse>
      ) : (
        <>
          <Alert mode='simple' type={hasCriticalWarnings ? 'error' : 'warning'}>
            {txSimulation.warnings[0].message}
          </Alert>
        </>
      )}
    </FullWidth>
  ) : null
}
