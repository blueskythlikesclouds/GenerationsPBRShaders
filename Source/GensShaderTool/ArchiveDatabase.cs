using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Amicitia.IO;
using Amicitia.IO.Binary;

namespace GensShaderTool
{
    public enum ConflictPolicy
    {
        RaiseError,
        Replace,
        Ignore
    }

    public class DatabaseData
    {
        public string Name { get; set; }
        public byte[] Data { get; set; }
        public DateTime Time { get; set; }

        public DatabaseData()
        {
            Time = DateTime.Now;
        }
    }

    public class ArchiveDatabase
    {
        public List<DatabaseData> Contents { get; }

        public void LoadSingle(string filePath)
        {
            using var reader = new BinaryValueReader(filePath, Endianness.Little, Encoding.UTF8);

            reader.Seek(16, SeekOrigin.Begin);
            while (reader.Position < reader.Length)
            {
                long currentOffset = reader.Position;
                long blockSize = reader.Read<uint>();
                long dataSize = reader.Read<uint>();
                long dataOffset = reader.Read<uint>();
                ulong modifiedTime = reader.Read<ulong>();
                string name = reader.ReadString(StringBinaryFormat.NullTerminated);

                reader.Seek(currentOffset + dataOffset, SeekOrigin.Begin);
                Contents.Add(new DatabaseData
                {
                    Name = name,
                    Data = reader.ReadArray<byte>((int)dataSize),
                    Time = modifiedTime != 0 ? DateTime.FromFileTime((long)(modifiedTime - 504911232000000000)) : DateTime.MinValue
                });

                reader.Seek(currentOffset + blockSize, SeekOrigin.Begin);
            }
        }

        public void Load(string filePath)
        {
            if (!filePath.EndsWith(".ar.00", StringComparison.OrdinalIgnoreCase))
            {
                LoadSingle(filePath);
                return;
            }

            filePath = filePath[..^2];

            for (int i = 0;; i++)
            {
                string currentFilePath = $"{filePath}{i:D2}";
                if (!File.Exists(currentFilePath))
                    break;

                LoadSingle(currentFilePath);
            }
        }

        public void Save(string filePath, int padding = 16, int maxSplitSize = 10 * 1024 * 1024)
        {
            bool splitMode = filePath.EndsWith(".ar.00", StringComparison.OrdinalIgnoreCase);

            if (splitMode)
                filePath = filePath[..^2];

            var fileSizes = new List<long>();

            BinaryValueWriter writer = null;

            for (var i = 0; i < Contents.Count; i++)
            {
                if (writer == null)
                {
                    writer = new BinaryValueWriter(
                        splitMode ? $"{filePath}{fileSizes.Count:D2}" : filePath, Endianness.Little, Encoding.UTF8);

                    writer.Write(0);
                    writer.Write(16);
                    writer.Write(20);
                    writer.Write(padding);
                }

                var databaseData = Contents[i];

                long currentOffset = writer.Position;
                long dataOffsetUnaligned = currentOffset + 21 + Encoding.UTF8.GetByteCount(databaseData.Name);
                long dataOffset = AlignmentHelper.Align(dataOffsetUnaligned, padding);
                long blockSize = dataOffset + databaseData.Data.Length;

                writer.Write((uint)(blockSize - currentOffset));
                writer.Write(databaseData.Data.Length);
                writer.Write((uint)(dataOffset - currentOffset));
                writer.Write(databaseData.Time != DateTime.MinValue ? (ulong)(databaseData.Time.ToFileTime() + 504911232000000000) : 0);
                writer.WriteString(StringBinaryFormat.NullTerminated, databaseData.Name);
                writer.Align(padding);
                writer.WriteBytes(databaseData.Data);

                if ((!splitMode || writer.Length <= maxSplitSize) && i != Contents.Count - 1)
                    continue;

                fileSizes.Add(writer.Length);

                writer.Dispose();
                writer = null;
            }

            if (!splitMode)
                return;

            writer = new BinaryValueWriter(filePath[..^1] + "l", Endianness.Little, Encoding.UTF8);

            writer.WriteString(StringBinaryFormat.FixedLength, "ARL2", 4);
            writer.Write((uint)fileSizes.Count);

            foreach (long fileSize in fileSizes)
                writer.Write((uint)fileSize);

            foreach (var databaseData in Contents)
                writer.WriteString(StringBinaryFormat.PrefixedLength8, databaseData.Name);

            writer.Dispose();
        }

        public void Add(DatabaseData databaseData, ConflictPolicy conflictPolicy = ConflictPolicy.RaiseError)
        {
            lock (((ICollection)Contents).SyncRoot)
            {
                var data = Contents.FirstOrDefault(x => x.Name.Equals(databaseData.Name));
                if (data != null)
                {
                    switch (conflictPolicy)
                    {
                        case ConflictPolicy.RaiseError:
                            throw new ArgumentException("Data with same name already exists", nameof(databaseData));

                        case ConflictPolicy.Replace:
                            Contents.Remove(data);
                            break;

                        case ConflictPolicy.Ignore:
                            return;
                    }
                }

                Contents.Add(databaseData);
            }
        }

        public void Sort()
        {
            Contents.Sort((x, y) => string.Compare(x.Name, y.Name, StringComparison.Ordinal));
        }

        public ArchiveDatabase()
        {
            Contents = new List<DatabaseData>();
        }

        public ArchiveDatabase(string filePath) : this()
        {
            Load(filePath);
        }
    }
}