// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'

// hooks
import {
  useSelectedPendingTransaction //
} from '../../../common/hooks/use-pending-transaction'

// components
import {
  ErcTokenApproval,
  EvmNativeAssetOrErc20TokenTransfer,
  NonFungibleErcTokenTransfer
} from './common/evm_state_changes'
import {
  SolStakingAuthChange,
  SOLTransfer,
  SPLTokenTransfer
} from './common/svm_state_changes'

// style
import { TransactionTitle, TransactionTypeText } from './style'
import {
  groupSimulatedEVMStateChanges,
  decodeSimulatedSVMStateChanges,
} from '../../../utils/tx-simulation-utils'
import { Column, Row } from '../../shared/style'
import {
  CollapseHeaderDivider,
  Divider,
  TransactionChangeCollapse,
  TransactionChangeCollapseContainer,
  TransactionChangeCollapseContent,
  TransactionChangeCollapseTitle
} from './confirm_simulated_tx_panel.styles'
import { ChainInfo } from './common/view_on_explorer_button'

type TransactionInfoProps = (
  | {
      simulationType: 'EVM'
      simulation: BraveWallet.EVMSimulationResponse
    }
  | {
      simulationType: 'SVM'
      simulation: BraveWallet.SolanaSimulationResponse
    }
) & {
  network: ChainInfo
}

export const TransactionSimulationInfo = ({
  simulation,
  simulationType,
  network
}: TransactionInfoProps) => {
  // custom hooks
  const tx = useSelectedPendingTransaction()

  // computed
  const { simulationResults } = simulation
  const { expectedStateChanges } = simulationResults
  const sendOptions = tx?.txDataUnion.solanaTxData?.sendOptions

  const { evmChanges, svmChanges } =
    simulationType === 'EVM'
      ? {
          evmChanges: groupSimulatedEVMStateChanges(
            expectedStateChanges as BraveWallet.BlowfishEVMStateChange[]
          ),
          svmChanges: undefined
        }
      : {
          svmChanges: decodeSimulatedSVMStateChanges(
            expectedStateChanges as BraveWallet.BlowfishSolanaStateChange[]
          ),
          evmChanges: undefined
        }

  const hasApprovals = Boolean(
    evmChanges?.evmApprovals.length || svmChanges?.splApprovals.length
  )

  const hasTransfers = Boolean(
    evmChanges?.evmTransfers.length || svmChanges?.svmTransfers.length
  )

  const hasSolStakingAuthChanges = Boolean(
    svmChanges?.solStakeAuthorityChanges.length
  )

  const hasMultipleCategories =
    [hasApprovals, hasTransfers, hasSolStakingAuthChanges].filter(Boolean)
      .length > 1

  // state
  const [isTransfersSectionOpen, setTransfersSectionOpen] = React.useState(true)
  const [isApprovalsSectionOpen, setIsApprovalsSectionOpen] = React.useState(
    !hasMultipleCategories
  )
  const [isSolStakingAuthSectionOpen, setIsSolStakingAuthSectionOpen] =
    React.useState(!hasMultipleCategories)

  // methods
  const onToggleTransfersSection = () => {
    if (!hasMultipleCategories) {
      return
    }

    if (isTransfersSectionOpen) {
      setTransfersSectionOpen(false)
      return
    }

    setTransfersSectionOpen(true)
    setIsApprovalsSectionOpen(false)
    setIsSolStakingAuthSectionOpen(false)
  }

  const onToggleApprovalsSection = () => {
    if (!hasMultipleCategories) {
      return
    }

    if (isApprovalsSectionOpen) {
      setIsApprovalsSectionOpen(false)
      return
    }

    setIsApprovalsSectionOpen(true)

    setTransfersSectionOpen(false)
    setIsSolStakingAuthSectionOpen(false)
  }

  const onToggleSolStakingAuthSection = () => {
    if (!hasMultipleCategories) {
      return
    }

    if (isSolStakingAuthSectionOpen) {
      setIsSolStakingAuthSectionOpen(false)
      return
    }

    setIsSolStakingAuthSectionOpen(true)

    setTransfersSectionOpen(false)
    setIsApprovalsSectionOpen(false)
  }

  // render
  return (
    <TransactionChangeCollapseContainer
      hasMultipleCategories={hasMultipleCategories}
    >
      {/* Transferred Assets */}
      {hasTransfers ? (
        <TransactionChangeCollapse
          onToggle={onToggleTransfersSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isTransfersSectionOpen}
          key={'transfers'}
        >
          <TransactionChangeCollapseTitle slot='title'>
            {
              // TODO: locale
              'Balance Changes'
            }
          </TransactionChangeCollapseTitle>
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'Transfers-Divider'} />

            {evmChanges?.evmTransfers.map((transfer, i, arr) => {
              return (
                <React.Fragment key={'EVM-Transfer-' + i}>
                  {getComponentForEvmTransfer(transfer, network)}
                  {i < arr.length - 1 ? <Divider /> : null}
                </React.Fragment>
              )
            })}

            {svmChanges?.svmTransfers.map((transfer, i, arr) => (
              <React.Fragment key={'SVM-Transfer' + i}>
                {getComponentForSvmTransfer(transfer, network)}
                {i < arr.length - 1 ? <Divider /> : null}
              </React.Fragment>
            ))}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      ) : null}

      {hasApprovals && (
        <TransactionChangeCollapse
          onToggle={onToggleApprovalsSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isApprovalsSectionOpen}
          key='approvals'
        >
          <TransactionChangeCollapseTitle slot='title'>
            {
              // TODO: locale
              'Approvals'
            }
          </TransactionChangeCollapseTitle>
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'EVM-Approvals-Divider'} />

            {evmChanges?.evmApprovals.map((approval, i, arr) => (
              <React.Fragment key={'EVM-Token-Approval-' + i}>
                {getComponentForEvmApproval(approval, network)}
                {i < arr.length - 1 ? <Divider /> : null}
              </React.Fragment>
            ))}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      )}

      {hasSolStakingAuthChanges && (
        <TransactionChangeCollapse
          onToggle={onToggleSolStakingAuthSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isSolStakingAuthSectionOpen}
          key='SOL-staking-changes'
        >
          <TransactionChangeCollapseTitle slot='title'>
            {
              // TODO: locale
              'SOL Staking changes'
            }
          </TransactionChangeCollapseTitle>
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'SolStakingAuthChanges-Divider'} />
            {svmChanges?.solStakeAuthorityChanges.map((approval, i, arr) =>
              approval.rawInfo.data.solStakeAuthorityChangeData ? (
                <React.Fragment key={'SolStakingAuthChanges-' + i}>
                  <SolStakingAuthChange
                    authChange={
                      approval.rawInfo.data.solStakeAuthorityChangeData
                    }
                    network={network}
                  />
                  {i < arr.length - 1 ? <Divider /> : null}
                </React.Fragment>
              ) : null
            )}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      )}

      {/* SEND OPTIONS */}
      {sendOptions && (
        <Column margin={'16px 4px 0px 4px'}>
          {!!Number(sendOptions?.maxRetries?.maxRetries) && (
            <Row justifyContent='flex-start' gap={'4px'}>
              <TransactionTitle>
                {getLocale('braveWalletSolanaMaxRetries')}
              </TransactionTitle>
              <TransactionTypeText>
                {Number(sendOptions?.maxRetries?.maxRetries)}
              </TransactionTypeText>
            </Row>
          )}

          {sendOptions?.preflightCommitment && (
            <Row justifyContent='flex-start' gap={'4px'}>
              <TransactionTitle>
                {getLocale('braveWalletSolanaPreflightCommitment')}
              </TransactionTitle>
              <TransactionTypeText>
                {sendOptions.preflightCommitment}
              </TransactionTypeText>
            </Row>
          )}

          {sendOptions?.skipPreflight && (
            <Row justifyContent='flex-start' gap={'4px'}>
              <TransactionTitle>
                {getLocale('braveWalletSolanaSkipPreflight')}
              </TransactionTitle>
              <TransactionTypeText>
                {sendOptions.skipPreflight.skipPreflight.toString()}
              </TransactionTypeText>
            </Row>
          )}
        </Column>
      )}
    </TransactionChangeCollapseContainer>
  )
}

function getComponentForEvmApproval(
  approval: BraveWallet.BlowfishEVMStateChange,
  network: ChainInfo
) {
  const { data } = approval.rawInfo

  if (data.erc20ApprovalData) {
    return (
      <ErcTokenApproval
        key={approval.humanReadableDiff}
        approval={data.erc20ApprovalData}
        network={network}
        isERC20={true}
      />
    )
  }

  if (data.erc1155ApprovalForAllData) {
    return (
      <ErcTokenApproval
        key={approval.humanReadableDiff}
        approval={data.erc1155ApprovalForAllData}
        network={network}
        isApprovalForAll={true}
      />
    )
  }

  if (data.erc721ApprovalData) {
    return (
      <ErcTokenApproval
        key={approval.humanReadableDiff}
        approval={data.erc721ApprovalData}
        network={network}
      />
    )
  }

  if (data.erc721ApprovalForAllData) {
    return (
      <ErcTokenApproval
        key={approval.humanReadableDiff}
        approval={data.erc721ApprovalForAllData}
        network={network}
        isApprovalForAll={true}
      />
    )
  }

  return null
}

function getComponentForSvmTransfer(
  transfer: BraveWallet.BlowfishSolanaStateChange,
  network: ChainInfo
) {
  const { data } = transfer.rawInfo

  if (data.solTransferData) {
    return (
      <SOLTransfer
        key={transfer.humanReadableDiff}
        transfer={data.solTransferData}
        network={network}
      />
    )
  }

  if (data.splTransferData) {
    return (
      <SPLTokenTransfer
        key={transfer.humanReadableDiff}
        transfer={data.splTransferData}
        network={network}
      />
    )
  }

  return null
}

function getComponentForEvmTransfer(
  transfer: BraveWallet.BlowfishEVMStateChange,
  network: ChainInfo
) {
  const { data } = transfer.rawInfo

  if (data.erc1155TransferData) {
    return (
      <NonFungibleErcTokenTransfer
        key={transfer.humanReadableDiff}
        transfer={data.erc1155TransferData}
        network={network}
      />
    )
  }

  if (data.erc20TransferData) {
    return (
      <EvmNativeAssetOrErc20TokenTransfer
        key={transfer.humanReadableDiff}
        transfer={data.erc20TransferData}
        network={network}
      />
    )
  }

  if (data.nativeAssetTransferData) {
    return (
      <EvmNativeAssetOrErc20TokenTransfer
        key={transfer.humanReadableDiff}
        transfer={data.nativeAssetTransferData}
        network={network}
      />
    )
  }

  if (data.erc721TransferData) {
    return (
      <NonFungibleErcTokenTransfer
        key={transfer.humanReadableDiff}
        transfer={data.erc721TransferData}
        network={network}
      />
    )
  }

  return null
}
