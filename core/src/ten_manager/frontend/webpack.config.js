//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
const path = require("path");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const { CleanWebpackPlugin } = require("clean-webpack-plugin");
const CopyWebpackPlugin = require("copy-webpack-plugin");

module.exports = {
  entry: "./src/index.tsx",
  mode: "development", // Or 'production' as needed.
  output: {
    path: path.resolve(__dirname, "dist"),
    filename: "bundle.js",
    publicPath: "/", // Ensure the routing is correct.
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
          from: path.resolve(__dirname, "public", "locales"), // Source folder.
          to: path.resolve(__dirname, "dist", "locales"), // Target folder.
        },
      ],
    }),
  ],
  devServer: {
    static: path.resolve(__dirname, "dist"),
    historyApiFallback: true, // Handle React Router.
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
