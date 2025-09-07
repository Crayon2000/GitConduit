package main

import (
	"flag"
	"fmt"
	"os"
)

func main() {
	// Define the main arguments
	cloneAndPushCmd := flag.NewFlagSet("cloneandpush", flag.ExitOnError)

	// Define flags for the cloneandpush command
	sourcerepo := cloneAndPushCmd.String("sourcerepo", "", "Source repository URL to clone")
	sourceusername := cloneAndPushCmd.String("sourceusername", "", "Source username for authentication")
	sourcepassword := cloneAndPushCmd.String("sourcepassword", "", "Source password for authentication")
	destrepo := cloneAndPushCmd.String("destrepo", "", "Destination repository URL to clone")
	destusername := cloneAndPushCmd.String("destusername", "", "Destination username for authentication")
	destpassword := cloneAndPushCmd.String("destpassword", "", "Destination password for authentication")
	hasWiki := cloneAndPushCmd.Bool("haswiki", false, "Set to true if the repository has a wiki (default: false)")

	if len(os.Args) < 2 {
		fmt.Println("Expected 'cloneandpush' subcommands")
		os.Exit(1)
	}

	switch os.Args[1] {
	case "cloneandpush":
		cloneAndPushCmd.Parse(os.Args[2:])
		if *sourcerepo == "" || *sourcepassword == "" || *destrepo == "" || *destpassword == "" {
			fmt.Println("Some arguments are missing for the cloneandpush command")
			os.Exit(1)
		}
		CloneAndPush(*sourcerepo, *sourceusername, *sourcepassword, *destrepo, *destusername, *destpassword, *hasWiki)

	default:
		fmt.Fprintln(os.Stderr, "Unknown command")
		os.Exit(1)
	}
}
