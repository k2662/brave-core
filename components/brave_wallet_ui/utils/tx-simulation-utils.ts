// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'
import { BLOWFISH_URL_WARNING_KINDS } from '../common/constants/blowfish'

interface GroupedEVMStateChanges {
  evmApprovals: BraveWallet.BlowfishEVMStateChange[]
  evmTransfers: BraveWallet.BlowfishEVMStateChange[]
}

interface GroupedSVMStateChanges {
  solStakeAuthorityChanges: BraveWallet.BlowfishSolanaStateChange[]
  splApprovals: BraveWallet.BlowfishSolanaStateChange[]
  svmTransfers: BraveWallet.BlowfishSolanaStateChange[]
}

export const groupSimulatedEVMStateChanges = (
  evmStateChanges: BraveWallet.BlowfishEVMStateChange[]
): GroupedEVMStateChanges => {
  const changes: GroupedEVMStateChanges = {
    evmApprovals: [],
    evmTransfers: []
  }

  for (const stateChange of evmStateChanges) {
    const { data } = stateChange.rawInfo

    // approvals & approvals for all
    if (
      data.erc20ApprovalData ||
      data.erc721ApprovalData ||
      data.erc721ApprovalForAllData ||
      data.erc1155ApprovalForAllData
    ) {
      changes.evmApprovals.push(stateChange)
    }

    // transfers
    if (
      data.erc20TransferData ||
      data.erc721TransferData ||
      data.erc1155TransferData ||
      data.nativeAssetTransferData
    ) {
      changes.evmTransfers.push(stateChange)
    }
  }

  return changes
}

export const decodeSimulatedSVMStateChanges = (
  stateChanges: BraveWallet.BlowfishSolanaStateChange[]
): GroupedSVMStateChanges => {
  const changes: GroupedSVMStateChanges = {
    solStakeAuthorityChanges: [],
    splApprovals: [],
    svmTransfers: [],
  }

  for (const stateChange of stateChanges) {
    const { data } = stateChange.rawInfo

    // TODO: account owner changes
    // if (data) {
    //   changes.svmStakeAuthorityChanges.push(
    //     stateChange as SafeSolanaAccountOwnerChangeEvent
    //   )
    // }

    // staking auth changes
    if (stateChange.rawInfo.data.solStakeAuthorityChangeData) {
      changes.solStakeAuthorityChanges.push(stateChange)
    }

    // approvals
    if (data.splApprovalData) {
      changes.splApprovals.push(stateChange)
    }

    // transfers
    if (data.solTransferData || data.splTransferData) {
      changes.svmTransfers.push(stateChange)
    }
  }

  return changes
}

export const isUrlWarning = (warningKind: BraveWallet.BlowfishWarningKind) => {
  return BLOWFISH_URL_WARNING_KINDS.includes(warningKind)
}

export const translateEvmSimulationResultError = (
  error:
    | BraveWallet.BlowfishEVMError
    | BraveWallet.BlowfishSolanaError
    | undefined
) => {
  if (!error) {
    return ''
  }

  switch (error?.kind) {
    case BraveWallet.BlowfishEVMErrorKind.kSimulationFailed:
      // TODO: getLocale('')
      return 'SIMULATION_FAILED'

    case BraveWallet.BlowfishEVMErrorKind.kTransactionError:
      // TODO: getLocale('')
      return 'TRANSACTION_ERROR'

    case BraveWallet.BlowfishEVMErrorKind.kTransactionReverted:
      // TODO: getLocale('')
      return 'TRANSACTION_REVERTED'

    case BraveWallet.BlowfishEVMErrorKind.kUnknownError:
    default:
      // TODO: getLocale('')
      return error.humanReadableError || 'UNKNOWN ERROR'
  }
}
