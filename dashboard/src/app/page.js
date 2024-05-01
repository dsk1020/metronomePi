"use client";
import React, { useState, useEffect } from 'react';

const Logo = () => {
  return (
    <h4 className="text-4xl font-bold mb-4">
      <span className="text-blue-700">Gator</span>
      <span className="text-orange-400">Byte</span> Metronome
    </h4>
  );
};
const updateBPM = () => {
  function handleUpdateBPM(e) {
    e.preventDefault();
    const form = e.target;
    const formData = new FormData(form);
    const bpm = formData.get("bpm");
    const bpmNumber = parseInt(bpm);
    fetch("http://192.168.0.104:8080/bpm", {
      method: "POST",
      headers: {
        "Content-Type": "application/json", // Specify JSON content type
      },
      body: JSON.stringify(bpmNumber),
    });
  }
  return (
    <form onSubmit={handleUpdateBPM}>
      <input type="number" name="bpm" className="border border-gray-400 p-2 mr-2 rounded-lg" />
      <button
        className="bg-blue-500 hover:bg-blue-700 text-white py-2 px-4 rounded"
        type="submit"
      >
        Update BPM
      </button>
    </form>
  );
};

const refreshButton = ({ onRefresh }) => {
  return (
    <button
      className="bg-blue-500 hover:bg-blue-700 text-white py-2 px-4 rounded"
      onClick={onRefresh}
    >
      Refresh
    </button>
  );
};

export default function Home() {
  const [minBPM, setMinBPM] = useState(0);
  const [maxBPM, setMaxBPM] = useState(0);
  const [currentBPM, setCurrentBPM] = useState(0);

  const fetchMinBPM = () => {
    fetch("http://192.168.0.104:8080/bpm/min", { method: "GET" })
      .then((res) => res.json())
      .then((data) => setMinBPM(data.bpm));
  };

  const fetchMaxBPM = () => {
    fetch("http://192.168.0.104:8080/bpm/max", { method: "GET" })
      .then((res) => res.json())
      .then((data) => setMaxBPM(data.bpm));
  };
  const fetchCurrentBPM = () => {
    fetch("http://192.168.0.104:8080/bpm", { method: "GET" })
      .then((res) => res.json())
      .then((data) => setCurrentBPM(data.bpm));
  };

  const deleteMinBPM = () => {
    fetch("http://192.168.0.104:8080/bpm/min", { method: "DELETE" })
      .then((res) => res.json())
      .then((data) => setMinBPM(data.bpm));
  }
  const deleteMaxBPM = () => {
    fetch("http://192.168.0.104:8080/bpm/max", { method: "DELETE" })
      .then((res) => res.json())
      .then((data) => setMaxBPM(data.bpm));
  }

  useEffect(() => {
    fetchMinBPM();
    fetchMaxBPM();
    fetchCurrentBPM();
  }, []);

  const handleRefresh = () => {
    fetchMinBPM();
    fetchMaxBPM();
    fetchCurrentBPM();
  };

  return (
    <main className="flex min-h-screen flex-col items-center p-24">
      {Logo()}
      <p className="mb-12">Control your metronome!</p>
      <div className="mb-12">{updateBPM()}</div>
      <div className="mb-12">
        <div className="text-blue-500 text-md text-center mb-4">Current BPM</div>
        <h1 className="text-2xl text-center mb-4">
          {currentBPM} <span className="text-grey-400 text-sm">BPM</span>
        </h1>
      </div>
      <div className="grid grid-cols-2 gap-20 mb-10">
        <div className="flex flex-col justify-center">
          <h4 className="text-red-500 text-md text-center">Lowest BPM</h4>
          <h1 className="text-2xl text-center mb-4">
            {minBPM} <span className="text-grey-400 text-sm">BPM</span>
          </h1>
          <button className="bg-red-500 rounded-lg py-2 text-xs text-white" onClick={deleteMinBPM}>Delete</button>
        </div>
        <div className="flex flex-col justify-center">
          <h4 className="text-emerald-500 text-md text-center">Highest BPM</h4>
          <h1 className="text-2xl text-center mb-4">
            {maxBPM} <span className="text-grey-400 text-sm">BPM</span>
          </h1>
          <button className="bg-red-500 rounded-lg py-2 text-xs text-white" onClick={deleteMaxBPM}>Delete</button>
        </div>
      </div>
      <div className="mb-12">{refreshButton({ onRefresh: handleRefresh })}</div>
    </main>
  );
}

