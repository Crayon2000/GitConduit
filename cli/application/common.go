package application

import (
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"regexp"
	"strings"
)

// GetOrganizations retrieves the list of organizations for the authenticated user.
func GetOrganizations(config AppConfig) ([]Organization, error) {
	req, err := http.NewRequest("GET", config.ApiUrl+"/user/orgs", nil)
	if err != nil {
		return nil, fmt.Errorf("failed to create request: %w", err)
	}
	req.Header.Set("Authorization", "token "+config.Token)
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	// GitBucket may return 404 if not supported
	if resp.StatusCode == http.StatusNotFound {
		return nil, nil // treat as "no orgs"
	}
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("unexpected status: %d", resp.StatusCode)
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, err
	}

	var rawOrgs []map[string]any
	if err := json.Unmarshal(body, &rawOrgs); err != nil {
		return nil, err
	}

	var orgs []Organization
	for _, obj := range rawOrgs {
		org := Organization{}
		// Try "login" or "username" for compatibility
		if login, ok := obj["login"].(string); ok {
			org.Name = login
		} else if username, ok := obj["username"].(string); ok {
			org.Name = username
		}
		if desc, ok := obj["description"].(string); ok {
			org.Description = desc
		}
		orgs = append(orgs, org)
	}
	return orgs, nil
}

// GetAuthenticatedUser retrieves the username of the authenticated user.
func GetAuthenticatedUser(config AppConfig) (string, error) {
	req, err := http.NewRequest("GET", config.ApiUrl+"/user", nil)
	if err != nil {
		return "", fmt.Errorf("failed to create request: %w", err)
	}
	req.Header.Set("Authorization", "token "+config.Token)
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)
	var data struct {
		Login string `json:"login"`
	}
	if err := json.Unmarshal(body, &data); err != nil {
		return "", err
	}
	return data.Login, nil
}

// GetNextUrl extracts the 'next' URL from the Link header for pagination.
func GetNextUrl(resp *http.Response) string {
	// Get the Link header
	linkHeader := resp.Header.Get("Link")
	if linkHeader == "" {
		return ""
	}
	// Regular expression to match the 'next' URL
	re := regexp.MustCompile(`<(\S+)>;\s*rel="next"`)
	match := re.FindStringSubmatch(linkHeader)
	if len(match) >= 2 {
		return match[1]
	}
	return ""
}

// JsonToUser parses a JSON object to fill a User struct.
func JsonToUser(obj map[string]any, u *User) error {
	if obj == nil {
		return errors.New("invalid JSON input for User")
	}
	if login, ok := obj["login"].(string); ok {
		u.Login = login
	}
	return nil
}

// JsonToRepo parses a JSON object to fill a Repository struct.
func JsonToRepo(obj map[string]any, repo *Repository) error {
	if obj == nil {
		return errors.New("invalid JSON input for Repository")
	}

	// Owner (required)
	if ownerObj, ok := obj["owner"].(map[string]any); ok {
		if err := JsonToUser(ownerObj, &repo.Owner); err != nil {
			return err
		}
	} else {
		return errors.New("owner not found")
	}

	// Name (required)
	if name, ok := obj["name"].(string); ok {
		repo.Name = name
	} else {
		return errors.New("name not found")
	}

	// FullName (required)
	if fullName, ok := obj["full_name"].(string); ok {
		repo.FullName = fullName
	} else {
		return errors.New("full_name not found")
	}

	// Private (required)
	if priv, ok := obj["private"].(bool); ok {
		repo.Private = priv
	} else {
		return errors.New("private not found")
	}

	// Description
	if desc, ok := obj["description"].(string); ok {
		repo.Description = desc
	} else {
		repo.Description = ""
	}

	// Fork
	if fork, ok := obj["fork"].(bool); ok {
		repo.Fork = fork
	} else {
		repo.Fork = false // Default is false
	}

	// CloneUrl (required)
	if cloneURL, ok := obj["clone_url"].(string); ok {
		repo.CloneUrl = cloneURL
	} else {
		return errors.New("clone_url not found")
	}

	// MirrorUrl
	if mirrorURL, ok := obj["mirror_url"].(string); ok {
		repo.MirrorUrl = mirrorURL
	} else {
		repo.MirrorUrl = ""
	}

	// OpenIssueCount
	if issues, ok := obj["open_issues_count"].(float64); ok {
		repo.OpenIssueCount = int(issues)
	} else {
		repo.OpenIssueCount = 0
	}

	// HasWiki (default true)
	if hasWiki, ok := obj["has_wiki"].(bool); ok {
		repo.HasWiki = hasWiki
	} else {
		repo.HasWiki = true
	}

	// HasIssues (default true)
	if hasIssues, ok := obj["has_issues"].(bool); ok {
		repo.HasIssues = hasIssues
	} else {
		repo.HasIssues = true
	}

	// HasProjects (default true)
	if hasProjects, ok := obj["has_projects"].(bool); ok {
		repo.HasProjects = hasProjects
	} else {
		repo.HasProjects = true
	}

	// HasDownloads (default true)
	if hasDownloads, ok := obj["has_downloads"].(bool); ok {
		repo.HasDownloads = hasDownloads
	} else {
		repo.HasDownloads = true
	}

	// Homepage
	if homepage, ok := obj["homepage"].(string); ok {
		repo.Homepage = homepage
	} else {
		repo.Homepage = ""
	}

	return nil
}

// GetRepositories retrieves repositories for a given owner and endpoint.
func GetRepositories(config AppConfig, endpoint ApiEndpoint, owner string, authUser string) ([]Repository, error) {
	url := config.ApiUrl
	if endpoint == EndpointOrganization {
		url += "/orgs/" + owner + "/repos"
	} else {
		if owner == authUser {
			url += "/user/repos"
		} else {
			url += "/users/" + owner + "/repos"
		}
	}

	var repos []Repository

	for url != "" {
		req, err := http.NewRequest("GET", url, nil)
		if err != nil {
			return nil, fmt.Errorf("failed to create request: %w", err)
		}
		req.Header.Set("Authorization", "token "+config.Token)
		resp, err := http.DefaultClient.Do(req)
		if err != nil {
			return nil, err
		}
		defer resp.Body.Close()
		body, _ := io.ReadAll(resp.Body)

		var obj []map[string]any
		if err := json.Unmarshal(body, &obj); err != nil {
			return nil, err
		}

		for _, item := range obj {
			repo := Repository{}
			JsonToRepo(item, &repo)
			if endpoint == EndpointUser && repo.Owner.Login != owner {
				continue
			}
			repos = append(repos, repo)
		}

		url = GetNextUrl(resp)
	}
	return repos, nil
}

// JsonToIssue parses a JSON object (map[string]interface{}) to fill an Issue struct.
func JsonToIssue(obj map[string]any, issue *Issue) error {
	if obj == nil {
		return errors.New("invalid JSON input")
	}

	// Title
	issue.Title = ""
	if title, ok := obj["title"].(string); ok {
		issue.Title = title
	}

	// Body
	issue.Body = ""
	if body, ok := obj["body"].(string); ok {
		issue.Body = body
	}

	// State
	issue.State = ""
	if state, ok := obj["state"].(string); ok {
		issue.State = state
	}

	// Number
	if number, ok := obj["number"].(float64); ok {
		issue.Number = int(number)
	} else {
		issue.Number = 0
	}

	return nil
}

// GetIssues retrieves issues for a given repository.
func GetIssues(config AppConfig, repo Repository) ([]Issue, error) {
	url := config.ApiUrl + "/repos/" + repo.Owner.Login + "/" + repo.Name + "/issues"
	var issues []Issue

	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return nil, fmt.Errorf("failed to create request: %w", err)
	}
	req.Header.Set("Authorization", "token "+config.Token)
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)

	var obj []map[string]any
	if err := json.Unmarshal(body, &obj); err != nil {
		return nil, err
	}

	for _, item := range obj {
		issue := Issue{}
		JsonToIssue(item, &issue)
		issues = append(issues, issue)
	}

	return issues, nil
}

// RepoToJson converts a Repository struct to a GitHub-compatible JSON string for the repo API.
// Only the allowed fields are included in the output JSON.
func RepoToJson(repo Repository) (string, error) {
	// Create a map with the relevant fields
	out := map[string]any{
		"name":          repo.Name,
		"description":   repo.Description,
		"homepage":      repo.Homepage,
		"private":       repo.Private,
		"has_issues":    repo.HasIssues,
		"has_projects":  repo.HasProjects,
		"has_wiki":      repo.HasWiki,
		"has_downloads": repo.HasDownloads,
	}

	// Marshal to JSON with indentation (for readability; remove for compact)
	jsonBytes, err := json.MarshalIndent(out, "", "  ")
	if err != nil {
		return "", err
	}
	return string(jsonBytes), nil
}

// CreateRepo creates a new repository.
func CreateRepo(config AppConfig, endpoint ApiEndpoint, owner string, source Repository) (Repository, error) {
	url := config.ApiUrl
	if endpoint == EndpointOrganization {
		url += "/orgs/" + owner + "/repos"
	} else {
		url += "/user/repos"
	}

	jsonData, err := RepoToJson(source)
	if err != nil {
		return Repository{}, err
	}

	req, err := http.NewRequest("POST", url, strings.NewReader(jsonData))
	if err != nil {
		return Repository{}, fmt.Errorf("failed to create request: %w", err)
	}
	req.Header.Set("Authorization", "token "+config.Token)
	req.Header.Set("Content-Type", "application/json")
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return Repository{}, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusCreated {
		return Repository{}, fmt.Errorf("failed to create repo: %s", resp.Status)
	}

	body, _ := io.ReadAll(resp.Body)
	var obj map[string]any
	if err := json.Unmarshal(body, &obj); err != nil {
		return Repository{}, err
	}
	var repo Repository
	if err := JsonToRepo(obj, &repo); err != nil {
		return Repository{}, err
	}
	return repo, nil
}
