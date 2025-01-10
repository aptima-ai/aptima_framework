//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import * as React from "react";
import { useTranslation } from "react-i18next";
import { BlocksIcon, ArrowBigRightDashIcon } from "lucide-react";

import Popup from "@/components/Popup/Popup";
import { Button } from "@/components/ui/Button";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/Tabs";
import { ReactFlowDataContext } from "@/context/ReactFlowDataContext";
import {
  DataTable as ConnectionDataTable,
  connectionColumns,
  extensionConnectionColumns1,
  extensionConnectionColumns2,
} from "@/components/DataTable/ConnectionTable";
import { dispatchCustomNodeActionPopup } from "@/utils/popup";

const DEFAULT_WIDTH = 800;
const DEFAULT_HEIGHT = 400;

export interface CustomNodeConnPopupProps {
  source: string;
  target?: string;
  onClose?: () => void;
}

const CustomNodeConnPopup: React.FC<CustomNodeConnPopupProps> = ({
  source,
  target,
  onClose,
}) => {
  const { t } = useTranslation();

  const titleMemo = React.useMemo(() => {
    if (source && !target) {
      return t("popup.customNodeConn.srcTitle", { source });
    }
    if (source && target) {
      return t("popup.customNodeConn.connectTitle", { source, target });
    }
    return t("popup.customNodeConn.title");
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [source, target]);

  return (
    <Popup
      title={titleMemo}
      onClose={() => onClose?.()}
      initialWidth={DEFAULT_WIDTH}
      initialHeight={DEFAULT_HEIGHT}
      resizable
    >
      <div className="flex flex-col gap-2 w-full h-[328px]">
        {source && target && (
          <EdgeInfoContent source={source} target={target} />
        )}
        {source && !target && <CustomNodeConnPopupContent source={source} />}
      </div>
    </Popup>
  );
};

export default CustomNodeConnPopup;

function EdgeInfoContent(props: { source: string; target: string }) {
  const { source, target } = props;

  const { edges } = React.useContext(ReactFlowDataContext);

  const [, rowsMemo] = React.useMemo(() => {
    const relatedEdges = edges.filter(
      (e) => e.source === source && e.target === target
    );
    const rows = relatedEdges.map((e) => ({
      id: e.id,
      type: e.data?.connectionType,
      source: e.source,
      target: e.target,
    }));
    return [relatedEdges, rows];
  }, [edges, source, target]);

  return (
    <>
      <div className="flex w-full items-center gap-2">
        <Button
          variant="outline"
          size="lg"
          onClick={() => dispatchCustomNodeActionPopup("connections", source)}
        >
          <BlocksIcon className="w-4 h-4" />
          <span>{source}</span>
        </Button>
        <ArrowBigRightDashIcon className="w-6 h-6" />
        <Button
          variant="outline"
          size="lg"
          onClick={() => dispatchCustomNodeActionPopup("connections", target)}
        >
          <BlocksIcon className="w-4 h-4" />
          <span>{target}</span>
        </Button>
      </div>
      <ConnectionDataTable
        columns={connectionColumns}
        data={rowsMemo}
        className="overflow-y-auto"
      />
    </>
  );
}

function CustomNodeConnPopupContent(props: { source: string }) {
  const { source } = props;
  const [flowDirection, setFlowDirection] = React.useState<
    "upstream" | "downstream"
  >("upstream");

  const { t } = useTranslation();

  const { edges } = React.useContext(ReactFlowDataContext);

  const [rowsMemo] = React.useMemo(() => {
    const relatedEdges = edges.filter((e) =>
      flowDirection === "upstream" ? e.target === source : e.source === source
    );
    const rows = relatedEdges.map((e) => ({
      id: e.id,
      type: e.data?.connectionType,
      upstream: flowDirection === "upstream" ? e.source : e.target,
      downstream: flowDirection === "upstream" ? e.source : e.target,
    }));
    return [rows, relatedEdges];
  }, [flowDirection, edges, source]);

  return (
    <>
      <Tabs
        value={flowDirection}
        onValueChange={(value) =>
          setFlowDirection(value as "upstream" | "downstream")
        }
        className="w-[400px]"
      >
        <TabsList>
          <TabsTrigger value="upstream">{t("action.upstream")}</TabsTrigger>
          <TabsTrigger value="downstream">{t("action.downstream")}</TabsTrigger>
        </TabsList>
      </Tabs>
      <ConnectionDataTable
        columns={
          flowDirection === "upstream"
            ? extensionConnectionColumns2
            : extensionConnectionColumns1
        }
        data={rowsMemo.map((row) => ({
          ...row,
          source: row.upstream,
          target: row.downstream,
        }))}
        className="overflow-y-auto"
      />
    </>
  );
}
