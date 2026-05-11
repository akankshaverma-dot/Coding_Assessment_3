// No package declaration - run from Q1 folder directly
import java.io.*;
import java.nio.file.*;
import java.util.*;

/**
 * LOG FILE PARSING - Java Coding Assignment
 *
 * Parses a log file and displays the most recent N log entries
 * filtered by log type (error, warning, info, debug).
 *
 * Usage:
 *   LogParser <filePath> [numberOfLines] [logTypes]
 *
 * Examples:
 *   LogParser app.log
 *   LogParser app.log 20
 *   LogParser app.log 20 error,warning
 */
public class LogParser {

    // Supported log types
    private static final Set<String> VALID_LOG_TYPES =
            new HashSet<>(Arrays.asList("error", "warning", "info", "debug"));

    private final String filePath;
    private final int numberOfLines;
    private final Set<String> logTypes;

    /**
     * Constructor with all parameters.
     *
     * @param filePath      Path to the log file (required)
     * @param numberOfLines Number of recent log lines to display (default: 10)
     * @param logTypes      Comma-separated log types to filter (default: "error")
     */
    public LogParser(String filePath, int numberOfLines, String logTypes) {
        this.filePath = filePath;
        this.numberOfLines = numberOfLines;
        this.logTypes = parseLogTypes(logTypes);
    }

    /** Parse and validate the log type string into a Set. */
    private Set<String> parseLogTypes(String logTypesStr) {
        Set<String> types = new HashSet<>();
        for (String t : logTypesStr.split(",")) {
            String trimmed = t.trim().toLowerCase();
            if (!VALID_LOG_TYPES.contains(trimmed)) {
                throw new IllegalArgumentException(
                        "Invalid log type: '" + trimmed +
                        "'. Valid types are: error, warning, info, debug.");
            }
            types.add(trimmed);
        }
        return types;
    }

    /**
     * Core method: reads the file from the end and collects matching log lines.
     *
     * @return List of matching log lines (most recent first)
     * @throws IOException if the file cannot be read
     */
    public List<String> parse() throws IOException {
        File file = new File(filePath);

        // Note 1: Raise exception if file path is invalid
        if (!file.exists() || !file.isFile()) {
            throw new FileNotFoundException("Invalid file path: " + filePath);
        }

        // Read all lines into memory
        List<String> allLines = Files.readAllLines(file.toPath());

        List<String> result = new ArrayList<>();

        // Note 4: Parse from end to get most recently added logs
        for (int i = allLines.size() - 1; i >= 0 && result.size() < numberOfLines; i--) {
            String line = allLines.get(i).trim();
            if (line.isEmpty()) continue;

            // Note 3: Support both plain ("error ...") and bracketed ("[ERROR] ...") formats
            String lineLower = line.toLowerCase();
            for (String type : logTypes) {
                if (lineLower.startsWith(type) || lineLower.startsWith("[" + type + "]")) {
                    result.add(line);
                    break;
                }
            }
        }

        return result;
    }

    /** Display parsed results. */
    public void display() throws IOException {
        List<String> logs = parse();
        if (logs.isEmpty()) {
            System.out.println("No matching log entries found.");
        } else {
            System.out.println("=== Filtered Log Output (" + logs.size() + " entries) ===");
            for (String log : logs) {
                System.out.println(log);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Main entry point
    // -------------------------------------------------------------------------
    public static void main(String[] args) {
        // Defaults
        String filePath = null;
        int numberOfLines = 10;       // default
        String logTypes   = "error";  // default

        if (args.length < 1) {
            System.err.println("Usage: LogParser <filePath> [numberOfLines] [logTypes]");
            System.err.println("  logTypes: comma-separated list of error,warning,info,debug");
            System.exit(1);
        }

        filePath = args[0];

        if (args.length >= 2) {
            try {
                numberOfLines = Integer.parseInt(args[1]);
                if (numberOfLines <= 0) throw new NumberFormatException();
            } catch (NumberFormatException e) {
                System.err.println("Error: numberOfLines must be a positive integer.");
                System.exit(1);
            }
        }

        if (args.length >= 3) {
            logTypes = args[2];
        }

        try {
            LogParser parser = new LogParser(filePath, numberOfLines, logTypes);
            parser.display();
        } catch (IllegalArgumentException e) {
            // Note 2: Raise exception if log type is invalid
            System.err.println("Error: " + e.getMessage());
            System.exit(1);
        } catch (FileNotFoundException e) {
            // Note 1: Raise exception if file path is invalid
            System.err.println("Error: " + e.getMessage());
            System.exit(1);
        } catch (IOException e) {
            System.err.println("Error reading file: " + e.getMessage());
            System.exit(1);
        }
    }
}