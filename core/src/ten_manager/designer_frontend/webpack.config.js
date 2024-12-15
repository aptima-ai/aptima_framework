//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
import { fileURLToPath } from "url";
import path from "path";
import HtmlWebpackPlugin from "html-webpack-plugin";
import { CleanWebpackPlugin } from "clean-webpack-plugin";
import CopyWebpackPlugin from "copy-webpack-plugin";
import ESLintPlugin from "eslint-webpack-plugin";
import MonacoWebpackPlugin from "monaco-editor-webpack-plugin";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export default {
  entry: "./src/index.tsx",
  mode: "development", // Or 'production' as needed.
  output: {
    path: path.resolve(__dirname, "dist"),
    filename: "bundle.js",
    publicPath: "/",
  },
  resolve: {
    extensions: [".tsx", ".ts", ".js"],
  },
  // For debugging purposes; can be removed in the production environment.
  devtool: "source-map",
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        use: "ts-loader",
        exclude: /node_modules/,
      },
      {
        test: /\.css$/i,
        use: ["style-loader", "css-loader"],
      },
      {
        test: /\.scss$/,
        use: ["style-loader", "css-loader", "sass-loader"],
      },
      // Other loaders (such as images, fonts) can be added here.
    ],
  },
  plugins: [
    new CleanWebpackPlugin(),
    new HtmlWebpackPlugin({
      template: "./public/index.html",
    }),
    new CopyWebpackPlugin({
      patterns: [
        {
          from: path.resolve(__dirname, "public", "locales"),
          to: path.resolve(__dirname, "dist", "locales"),
        },
      ],
    }),
    new ESLintPlugin({
      extensions: ["js", "jsx", "ts", "tsx"],
      emitWarning: true,
      failOnError: false,
      configType: "flat",
      overrideConfigFile: path.resolve(__dirname, "eslint.config.mjs"),
    }),
    new MonacoWebpackPlugin({
      // Only the resources of those languages needed.
      languages: ["python", "go", "cpp", "javascript", "typescript", "json"],
    }),
  ],
  devServer: {
    static: path.resolve(__dirname, "dist"),
    historyApiFallback: true,
    port: 3000,
    proxy: [
      {
        context: ["/api"],
        // If the dev-server is started on a different port (default is 49483),
        // corresponding changes need to be made here as well.
        target: "http://localhost:49483",
        changeOrigin: true,
        secure: false,
      },
    ],
  },
};
